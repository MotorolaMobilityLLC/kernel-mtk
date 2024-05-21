/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 MediaTek Inc.
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

#ifdef CONFIG_MTK_PMIC_WRAP_HAL
#include "pwrap_hal.h"
#endif

#if defined(CONFIG_MACH_MT6739)
void vmd1_pmic_setting_on(void)
{
	unsigned int vcore_vosel = 0;
	unsigned int vmodem_vosel = 0;
	unsigned int segment = get_devinfo_with_index(28);
	unsigned char vcore_segment = (unsigned char)((segment & 0xC0000000) >> 30);
	unsigned char vmodem_segment = (unsigned char)((segment & 0x08000000) >> 27);

	if ((vcore_segment & 0x3) == 0x1 ||
	    (vcore_segment & 0x3) == 0x2) {
		/* VCORE 1.15V: 0x65 */
		vcore_vosel = 0x65;
	} else {
		/* VCORE 1.20V: 0x6D */
		vcore_vosel = 0x6D;
	}

#if defined(CONFIG_BUILD_ARM64_APPENDED_DTB_IMAGE_NAMES)
	/* PMIC special flavor project */
	if (strncmp(CONFIG_BUILD_ARM64_APPENDED_DTB_IMAGE_NAMES,
		    "mediatek/k39tv1_ctightening", 27) == 0) {
		if ((vcore_segment & 0x3) == 0x1 ||
		    (vcore_segment & 0x3) == 0x2) {
			/* VCORE 1.11250V: 0x5F */
			vcore_vosel = 0x5F;
		} else {
			/* VCORE 1.15625V: 0x66 */
			vcore_vosel = 0x66;
		}
	} else if (strncmp(CONFIG_BUILD_ARM64_APPENDED_DTB_IMAGE_NAMES,
		    "mediatek/k39v1_ctightening", 26) == 0) {
		if ((vcore_segment & 0x3) == 0x1 ||
		    (vcore_segment & 0x3) == 0x2) {
			/* VCORE 1.0875V: 0x5B */
			vcore_vosel = 0x5B;
		} else {
			/* VCORE 1.13125V: 0x62 */
			vcore_vosel = 0x62;
		}
	} else if (strncmp(CONFIG_BUILD_ARM64_APPENDED_DTB_IMAGE_NAMES,
		    "mediatek/k39v1_bsp_ctighten", 27) == 0) {
		if ((vcore_segment & 0x3) == 0x1 ||
		    (vcore_segment & 0x3) == 0x2) {
			/* VCORE 1.0875V: 0x5B */
			vcore_vosel = 0x5B;
		} else {
			/* VCORE 1.13125V: 0x62 */
			vcore_vosel = 0x62;
		}
	}
#endif

	if (!vmodem_segment)
		vmodem_vosel = 0x6F;/* VMODEM 1.19375V: 0x6F */
	else
		vmodem_vosel = 0x68;/* VMODEM 1.15V: 0x68 */

	pr_notice("%s vcore vosel = 0x%x, da_vosel = 0x%x", __func__,
		pmic_get_register_value(PMIC_RG_BUCK_VCORE_VOSEL),
		pmic_get_register_value(PMIC_DA_VCORE_VOSEL));

	/* 1.Call PMIC driver API configure VMODEM voltage */
	pmic_set_register_value(PMIC_RG_BUCK_VMODEM_VOSEL, vmodem_vosel);
	pr_notice("%s vmodem vosel = 0x%x, da_vosel = 0x%x", __func__,
		pmic_get_register_value(PMIC_RG_BUCK_VMODEM_VOSEL),
		pmic_get_register_value(PMIC_DA_VMODEM_VOSEL));
}
#else
void vmd1_pmic_setting_on(void)
{
	/* Vcore: 0x2D, 0.8V  */
	/* Vmodem: 0x30, 0.8V */
	unsigned int vcore_vosel = 0x2D, vmodem_vosel = 0x30;

	if (pmic_get_register_value(PMIC_DA_VCORE_VOSEL) != vcore_vosel)
		pr_notice("%s vcore vosel = 0x%x, da_vosel = 0x%x",
		__func__, pmic_get_register_value(PMIC_RG_BUCK_VCORE_VOSEL),
		pmic_get_register_value(PMIC_DA_VCORE_VOSEL));

	/* 1.Call PMIC driver API configure VMODEM voltage */
	pmic_set_register_value(PMIC_RG_BUCK_VMODEM_VOSEL, vmodem_vosel);
	if (pmic_get_register_value(PMIC_DA_VMODEM_VOSEL) != vmodem_vosel)
		pr_notice("%s vmodem vosel = 0x%x, da_vosel = 0x%x",
		__func__, pmic_get_register_value(PMIC_RG_BUCK_VMODEM_VOSEL),
		pmic_get_register_value(PMIC_DA_VMODEM_VOSEL));
}
#endif /* CONFIG_MACH_MT6739 */

void vmd1_pmic_setting_off(void)
{
	PMICLOG("%s\n", __func__);
}

void wk_pmic_enable_sdn_delay(void)
{
	pmic_set_register_value(PMIC_TMA_KEY, 0x9CA8);
	pmic_set_register_value(PMIC_RG_SDN_DLY_ENB, 0);
	pmic_set_register_value(PMIC_TMA_KEY, 0);
}

int vcore_pmic_set_mode(unsigned char mode)
{
	unsigned char ret = 0;

	pmic_set_register_value(PMIC_RG_VCORE_FPWM, mode);

	ret = pmic_get_register_value(PMIC_RG_VCORE_FPWM);

	return (ret == mode) ? (0) : (-1);
}

int vproc_pmic_set_mode(unsigned char mode)
{
	unsigned char ret = 0;

	pmic_set_register_value(PMIC_RG_VPROC_FPWM, mode);

	ret = pmic_get_register_value(PMIC_RG_VPROC_FPWM);

	return (ret == mode) ? (0) : (-1);
}
/* [Export API] */
/* Support for MT6357 CRV/MRV */
unsigned int is_pmic_mrv(void)
{
	unsigned int g_is_mrv = 0;

	pmic_read_interface(PMIC_RG_TOP2_RSV0_ADDR,
			    &g_is_mrv, 0x1, 15);

	return g_is_mrv;
}

void pmic_enable_smart_reset(unsigned char smart_en, unsigned char smart_sdn_en)
{
	pmic_set_register_value(PMIC_RG_SMART_RST_MODE, smart_en);
	pmic_set_register_value(PMIC_RG_SMART_RST_SDN_EN, smart_sdn_en);
	pr_info("[%s] smart_en:%d, smart_sdn_en:%d\n",
		__func__, smart_en, smart_sdn_en);
}

int pmic_tracking_init(void)
{
	int ret = 0;
#if 0
	/* 0.1V */
	pmic_set_register_value(PMIC_RG_VSRAM_VCORE_VOSEL_OFFSET, 0x10);
	/* 0.025V */
	pmic_set_register_value(PMIC_RG_VSRAM_VCORE_VOSEL_DELTA, 0x4);
	/* 1.0V */
	pmic_set_register_value(PMIC_RG_VSRAM_VCORE_VOSEL_ON_HB, 0x60);
	/* 0.8V */
	pmic_set_register_value(PMIC_RG_VSRAM_VCORE_VOSEL_ON_LB, 0x40);
	/* 0.65V */
	pmic_set_register_value(PMIC_RG_VSRAM_VCORE_VOSEL_SLEEP_LB, 0x28);
#endif
#if 0
	ret = enable_vsram_vcore_hw_tracking(1);
	PMICLOG("Enable VSRAM_VCORE hw tracking\n");
#else
	PMICLOG("VSRAM_VCORE hw tracking not support in MT6357\n");
#endif

	return ret;
}


/*****************************************************************************
 * PMIC charger detection
 ******************************************************************************/
unsigned int upmu_get_rgs_chrdet(void)
{
	unsigned int val = 0;

	val = pmic_get_register_value(PMIC_RGS_CHRDET);
	PMICLOG("[%s] CHRDET status = %d\n", __func__, val);

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

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
#include <generated/autoconf.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/irqflags.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
/*
#if !defined CONFIG_HAS_WAKELOCKS
#include <linux/pm_wakeup.h>  included in linux/device.h
#else
*/
#include <linux/wakelock.h>
/*#endif*/
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <mt-plat/aee.h>
#include <linux/proc_fs.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/writeback.h>
#include <linux/seq_file.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/regulator/of_regulator.h>
#include <linux/of_device.h>
#include <linux/of_fdt.h>
#endif
#include <asm/uaccess.h>

#if !defined CONFIG_MTK_LEGACY
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#endif
#include <mt-plat/upmu_common.h>
#include "pmic.h"
#include "pmic_irq.h"
/*#include <mach/eint.h> TBD*/
#include <mach/mt_pmic_wrap.h>
#if defined CONFIG_MTK_LEGACY
#include <mt-plat/mt_gpio.h>
#endif
#include <mt-plat/mtk_rtc.h>
#include <mach/mt_spm_mtcmos.h>

#include <linux/time.h>

#include "pmic_dvt.h"

#if defined(CONFIG_MTK_KERNEL_POWER_OFF_CHARGING)
#include <mt-plat/mt_boot.h>
#include <mt-plat/mt_boot_common.h>
/*#include <mach/system.h> TBD*/
#include <mt-plat/mt_gpt.h>
#endif

#if defined(CONFIG_MTK_SMART_BATTERY)
#include <mt-plat/battery_meter.h>
#include <mt-plat/battery_common.h>
#include <mach/mt_battery_meter.h>
#endif
#include "mt6311.h"
#include <mach/mt_pmic.h>
#include <mt-plat/mt_reboot.h>
#include <mach/mt_charging.h>

/*****************************************************************************
 * PMIC extern variable
 ******************************************************************************/
static struct mtk_regulator mtk_bucks[];
/*****************************************************************************
 * Global variable
 ******************************************************************************/
unsigned int g_pmic_pad_vbif28_vol = 1;
/*****************************************************************************
 * PMIC related define
 ******************************************************************************/
static DEFINE_MUTEX(pmic_lock_mutex);
#define PMIC_EINT_SERVICE

/*****************************************************************************
 * PMIC read/write APIs
 ******************************************************************************/
#if 0				/*defined(CONFIG_MTK_FPGA)*/
    /* no CONFIG_PMIC_HW_ACCESS_EN */
#else
#define CONFIG_PMIC_HW_ACCESS_EN
#endif
/*
#define PMICTAG                "[PMIC] "
#if defined PMIC_DEBUG_PR_DBG
#define PMICLOG(fmt, arg...)   pr_err(PMICTAG fmt, ##arg)
#else
#define PMICLOG(fmt, arg...)
#endif
*/
#define PMIC_UT 0
#ifdef CONFIG_OF
#if !defined CONFIG_MTK_LEGACY
/*
static int pmic_mt_cust_probe(struct platform_device *pdev);
static int pmic_mt_cust_remove(struct platform_device *pdev);
static int pmic_regulator_ldo_init(struct platform_device *pdev);
*/
#endif
#endif				/* End of #ifdef CONFIG_OF */

static DEFINE_MUTEX(pmic_access_mutex);
/*--- Global suspend state ---*/
static bool pmic_suspend_state;

int pmic_force_vcore_pwm(bool enable)
{
	int val, val1, val2, ret;

	ret = pmic_read_interface_nolock(0x44e, &val, 0xFFFF, 0x0);
	ret = pmic_read_interface_nolock(0x450, &val1, 0xFFFF, 0x0);
	ret = pmic_read_interface_nolock(0x452, &val2, 0xFFFF, 0x0);
	pr_err("[pmic]pre force_vcore_pwm:0x%x, 0x%x, 0x%x\n", val, val1, val2);

	if (enable == true) {
		ret = pmic_config_interface_nolock(0x450, 0x0, 0x7, 0);
		ret = pmic_config_interface_nolock(0x452, 0x0, 0x1, 3);
		ret = pmic_config_interface_nolock(0x44e, 0x1, 0x1, 1);
	} else {
		ret = pmic_config_interface_nolock(0x44e, 0x0, 0x1, 1);
		ret = pmic_config_interface_nolock(0x450, 0x4, 0x7, 0);
		ret = pmic_config_interface_nolock(0x452, 0x1, 0x1, 3);
	}

	ret = pmic_read_interface_nolock(0x44e, &val, 0xFFFF, 0x0);
	ret = pmic_read_interface_nolock(0x450, &val1, 0xFFFF, 0x0);
	ret = pmic_read_interface_nolock(0x452, &val2, 0xFFFF, 0x0);
	pr_err("[pmic]post force_vcore_pwm:0x%x, 0x%x, 0x%x\n", val, val1, val2);

	return 0;
}

void vmd1_pmic_setting_on(void)
{
	/* VSRAM_MD on */
	pmic_set_register_value(MT6351_PMIC_BUCK_VSRAM_MD_EN, 1); /* 0x0654[0]=0, 0:Disable, 1:Enable */
	pmic_set_register_value(MT6351_PMIC_BUCK_VSRAM_MD_VSLEEP_EN, 1); /* 0x0662[8]=0, 0:SW control, 1:HW control */

	/* VMD1 on */
	pmic_set_register_value(MT6351_PMIC_BUCK_VMD1_EN, 1); /* 0x0640[0]=0, 0:Disable, 1:Enable */
	pmic_set_register_value(MT6351_PMIC_BUCK_VMD1_VSLEEP_EN, 1); /* 0x064E[8]=0, 0:SW control, 1:HW control */

	/* VMODEM on */
	pmic_set_register_value(MT6351_PMIC_BUCK_VMODEM_EN, 1); /* 0x062C[0]=0, 0:Disable, 1:Enable */
	pmic_set_register_value(MT6351_PMIC_BUCK_VMODEM_VSLEEP_EN, 1); /* 0x063A[8]=0, 0:SW control, 1:HW control */

	pmic_set_register_value(MT6351_PMIC_BUCK_VSRAM_MD_VOSEL_ON, 0x50);/*E1 1.0V; offset:0x65A */
	pmic_set_register_value(MT6351_PMIC_BUCK_VMODEM_VOSEL_ON, 0x40);/* 1.0V; offset: 0x632 */
	udelay(300);

	pmic_set_register_value(MT6351_PMIC_BUCK_VSRAM_MD_VOSEL_CTRL, 1);/* HW mode, bit[1]; offset: 0x650 */
	pmic_set_register_value(MT6351_PMIC_BUCK_VMD1_VOSEL_CTRL, 1);/* HW mode, bit[1]; offset: 0x63C */
	pmic_set_register_value(MT6351_PMIC_BUCK_VMODEM_VOSEL_CTRL, 1);/* HW mode, bit[1]; offset: 0x628 */
}

unsigned int pmic_read_vbif28_volt(unsigned int *val)
{
	if (g_pmic_pad_vbif28_vol != 0x1) {
		*val = g_pmic_pad_vbif28_vol;
		return 1;
	} else
		return 0;
}

unsigned int pmic_get_vbif28_volt(void)
{
	return g_pmic_pad_vbif28_vol;
}

unsigned int pmic_read_interface(unsigned int RegNum, unsigned int *val, unsigned int MASK, unsigned int SHIFT)
{
	unsigned int return_value = 0;

#if defined(CONFIG_PMIC_HW_ACCESS_EN)
	unsigned int pmic_reg = 0;
	unsigned int rdata;

	if ((pmic_suspend_state == true) && irqs_disabled())
		return pmic_read_interface_nolock(RegNum, val, MASK, SHIFT);

	mutex_lock(&pmic_access_mutex);

	/*mt_read_byte(RegNum, &pmic_reg);*/
	return_value = pwrap_wacs2(0, (RegNum), 0, &rdata);
	pmic_reg = rdata;
	if (return_value != 0) {
		PMICLOG("[pmic_read_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
		mutex_unlock(&pmic_access_mutex);
		return return_value;
	}
	/*PMICLOG"[pmic_read_interface] Reg[%x]=0x%x\n", RegNum, pmic_reg);*/

	pmic_reg &= (MASK << SHIFT);
	*val = (pmic_reg >> SHIFT);
	/*PMICLOG"[pmic_read_interface] val=0x%x\n", *val);*/

	mutex_unlock(&pmic_access_mutex);
#else
	/*PMICLOG("[pmic_read_interface] Can not access HW PMIC\n");*/
#endif

	return return_value;
}

int en_buck_vsleep_dbg = 0;
unsigned int pmic_buck_vsleep_to_swctrl(unsigned int buck_num, unsigned int vsleep_addr,
	unsigned int vsleep_mask, unsigned int vsleep_shift)
{
	unsigned int return_value = 0;
	unsigned int pmic_reg = 0;
	unsigned int rdata;

	return_value = pwrap_wacs2(0, (vsleep_addr), 0, &rdata);
	pmic_reg = rdata;
	mtk_bucks[buck_num].vsleep_en_saved = rdata;
	if (en_buck_vsleep_dbg == 1) {
		pr_err("[pmic]vsleep %s_vsleep[0x%x]=0x%x, 0x%x\n", mtk_bucks[buck_num].desc.name,
		vsleep_addr, mtk_bucks[buck_num].vsleep_en_saved, rdata);
	}
	pmic_reg &= ~(vsleep_mask << vsleep_shift);
	pmic_reg |= (0 << vsleep_shift);

	return_value = pwrap_wacs2(1, (vsleep_addr), pmic_reg, &rdata);
	if (en_buck_vsleep_dbg == 1) {
		udelay(1000);
		return_value = pwrap_wacs2(0, (vsleep_addr), 0, &rdata);
		pr_err("[pmic]vsleep.b %s_vsleep[0x%x]=0x%x, 0x%x\n", mtk_bucks[buck_num].desc.name,
			vsleep_addr, mtk_bucks[buck_num].vsleep_en_saved, pmic_reg);
	}
	return 0;
}
unsigned int pmic_buck_vsleep_restore(unsigned int buck_num, unsigned int vsleep_addr,
	unsigned int vsleep_mask, unsigned int vsleep_shift)
{
	unsigned int return_value = 0;
	unsigned int pmic_reg = 0;
	unsigned int rdata = 0;

	if (mtk_bucks[buck_num].vsleep_en_saved != 0x0fffffff) {
		if (vsleep_addr == MT6351_PMIC_BUCK_VGPU_VSLEEP_EN_ADDR)
			mtk_bucks[buck_num].vsleep_en_saved |= 0x13;
		pmic_reg &= ~(vsleep_mask << vsleep_shift);
		pmic_reg |= (mtk_bucks[buck_num].vsleep_en_saved);
		return_value = pwrap_wacs2(1, (vsleep_addr), pmic_reg, &rdata);
		if (en_buck_vsleep_dbg == 1) {
			pr_err("[pmic]restore %s_vsleep[0x%x]=0x%x, 0x%x\n", mtk_bucks[buck_num].desc.name,
			vsleep_addr, mtk_bucks[buck_num].vsleep_en_saved, rdata);
		}
		mtk_bucks[buck_num].vsleep_en_saved = 0x0fffffff;
	} else {
		if (en_buck_vsleep_dbg == 1) {
			pr_err("[pmic]bypass %s_vsleep[0x%x]=0x%x, 0x%x\n", mtk_bucks[buck_num].desc.name,
			vsleep_addr, mtk_bucks[buck_num].vsleep_en_saved, rdata);
		}
	}
	return 0;
}
unsigned int pmic_config_interface_buck_vsleep_check(unsigned int RegNum,
	unsigned int val, unsigned int MASK, unsigned int SHIFT)
{

	switch (RegNum) {
	case MT6351_PMIC_BUCK_VCORE_EN_ADDR:
		if ((val & 0x1) == 0x0) { /* buck off */
			pmic_buck_vsleep_to_swctrl(0,
				MT6351_PMIC_BUCK_VCORE_VSLEEP_EN_ADDR,
				MT6351_PMIC_BUCK_VCORE_VSLEEP_EN_MASK,
				MT6351_PMIC_BUCK_VCORE_VSLEEP_EN_SHIFT);
		} else if ((val & 0x1) == 0x1) { /* buck on */
			pmic_buck_vsleep_restore(0,
				MT6351_PMIC_BUCK_VCORE_VSLEEP_EN_ADDR,
				MT6351_PMIC_BUCK_VCORE_VSLEEP_EN_MASK,
				MT6351_PMIC_BUCK_VCORE_VSLEEP_EN_SHIFT);
		}
		break;
	case MT6351_PMIC_BUCK_VGPU_EN_ADDR:
		if ((val & 0x1) == 0x0) {
			pmic_buck_vsleep_to_swctrl(1,
				MT6351_PMIC_BUCK_VGPU_VSLEEP_EN_ADDR,
				MT6351_PMIC_BUCK_VGPU_VSLEEP_EN_MASK,
				MT6351_PMIC_BUCK_VGPU_VSLEEP_EN_SHIFT);
		} else if ((val & 0x1) == 0x1) {
			pmic_buck_vsleep_restore(1,
				MT6351_PMIC_BUCK_VGPU_VSLEEP_EN_ADDR,
				MT6351_PMIC_BUCK_VGPU_VSLEEP_EN_MASK,
				MT6351_PMIC_BUCK_VGPU_VSLEEP_EN_SHIFT);
		}
		break;
	case MT6351_PMIC_BUCK_VMODEM_EN_ADDR:
		if ((val & 0x1) == 0x0) {
			pmic_buck_vsleep_to_swctrl(2,
				MT6351_PMIC_BUCK_VMODEM_VSLEEP_EN_ADDR,
				MT6351_PMIC_BUCK_VMODEM_VSLEEP_EN_MASK,
				MT6351_PMIC_BUCK_VMODEM_VSLEEP_EN_SHIFT);

		} else if ((val & 0x1) == 0x1) {
			pmic_buck_vsleep_restore(2,
				MT6351_PMIC_BUCK_VMODEM_VSLEEP_EN_ADDR,
				MT6351_PMIC_BUCK_VMODEM_VSLEEP_EN_MASK,
				MT6351_PMIC_BUCK_VMODEM_VSLEEP_EN_SHIFT);
		}
		break;
	case MT6351_PMIC_BUCK_VMD1_EN_ADDR:
		if ((val & 0x1) == 0x0) {
			pmic_buck_vsleep_to_swctrl(3,
				MT6351_PMIC_BUCK_VMD1_VSLEEP_EN_ADDR,
				MT6351_PMIC_BUCK_VMD1_VSLEEP_EN_MASK,
				MT6351_PMIC_BUCK_VMD1_VSLEEP_EN_SHIFT);

		} else if ((val & 0x1) == 0x1) {
			pmic_buck_vsleep_restore(3,
				MT6351_PMIC_BUCK_VMD1_VSLEEP_EN_ADDR,
				MT6351_PMIC_BUCK_VMD1_VSLEEP_EN_MASK,
				MT6351_PMIC_BUCK_VMD1_VSLEEP_EN_SHIFT);
		}
		break;
	case MT6351_PMIC_BUCK_VSRAM_MD_EN_ADDR:
		if ((val & 0x1) == 0x0) {
			pmic_buck_vsleep_to_swctrl(4,
				MT6351_PMIC_BUCK_VSRAM_MD_VSLEEP_EN_ADDR,
				MT6351_PMIC_BUCK_VSRAM_MD_VSLEEP_EN_MASK,
				MT6351_PMIC_BUCK_VSRAM_MD_VSLEEP_EN_SHIFT);

		} else if ((val & 0x1) == 0x1) {
			pmic_buck_vsleep_restore(4,
				MT6351_PMIC_BUCK_VSRAM_MD_VSLEEP_EN_ADDR,
				MT6351_PMIC_BUCK_VSRAM_MD_VSLEEP_EN_MASK,
				MT6351_PMIC_BUCK_VSRAM_MD_VSLEEP_EN_SHIFT);
		}
		break;
	case MT6351_PMIC_BUCK_VS1_EN_ADDR:
		if ((val & 0x1) == 0x0) {
			pmic_buck_vsleep_to_swctrl(5,
				MT6351_PMIC_BUCK_VS1_VSLEEP_EN_ADDR,
				MT6351_PMIC_BUCK_VS1_VSLEEP_EN_MASK,
				MT6351_PMIC_BUCK_VS1_VSLEEP_EN_SHIFT);

		} else if ((val & 0x1) == 0x1) {
			pmic_buck_vsleep_restore(5,
				MT6351_PMIC_BUCK_VS1_VSLEEP_EN_ADDR,
				MT6351_PMIC_BUCK_VS1_VSLEEP_EN_MASK,
				MT6351_PMIC_BUCK_VS1_VSLEEP_EN_SHIFT);
		}
		break;
	case MT6351_PMIC_BUCK_VS2_EN_ADDR:
		if ((val & 0x1) == 0x0) {
			pmic_buck_vsleep_to_swctrl(6,
				MT6351_PMIC_BUCK_VS2_VSLEEP_EN_ADDR,
				MT6351_PMIC_BUCK_VS2_VSLEEP_EN_MASK,
				MT6351_PMIC_BUCK_VS2_VSLEEP_EN_SHIFT);
		} else if ((val & 0x1) == 0x1) {
			pmic_buck_vsleep_restore(6,
				MT6351_PMIC_BUCK_VS2_VSLEEP_EN_ADDR,
				MT6351_PMIC_BUCK_VS2_VSLEEP_EN_MASK,
				MT6351_PMIC_BUCK_VS2_VSLEEP_EN_SHIFT);
		}

		break;
	case MT6351_PMIC_BUCK_VPA_EN_ADDR:
		if ((val & 0x1) == 0x0) {
			pmic_buck_vsleep_to_swctrl(7,
				MT6351_PMIC_BUCK_VPA_VSLEEP_EN_ADDR,
				MT6351_PMIC_BUCK_VPA_VSLEEP_EN_MASK,
				MT6351_PMIC_BUCK_VPA_VSLEEP_EN_SHIFT);

		} else if ((val & 0x1) == 0x1) {
			pmic_buck_vsleep_restore(7,
				MT6351_PMIC_BUCK_VPA_VSLEEP_EN_ADDR,
				MT6351_PMIC_BUCK_VPA_VSLEEP_EN_MASK,
				MT6351_PMIC_BUCK_VPA_VSLEEP_EN_SHIFT);
		}
		break;
	case MT6351_PMIC_BUCK_VSRAM_PROC_EN_ADDR:
		if ((val & 0x1) == 0x0) {
			pmic_buck_vsleep_to_swctrl(8,
				MT6351_PMIC_BUCK_VSRAM_PROC_VSLEEP_EN_ADDR,
				MT6351_PMIC_BUCK_VSRAM_PROC_VSLEEP_EN_MASK,
				MT6351_PMIC_BUCK_VSRAM_PROC_VSLEEP_EN_SHIFT);

		} else if ((val & 0x1) == 0x1) {
			pmic_buck_vsleep_restore(8,
				MT6351_PMIC_BUCK_VSRAM_PROC_VSLEEP_EN_ADDR,
				MT6351_PMIC_BUCK_VSRAM_PROC_VSLEEP_EN_MASK,
				MT6351_PMIC_BUCK_VSRAM_PROC_VSLEEP_EN_SHIFT);
		}
		break;
	default:
		/* bypass buck vsleep en check */
		break;
	}
	return 0;
}

unsigned int pmic_config_interface(unsigned int RegNum, unsigned int val, unsigned int MASK, unsigned int SHIFT)
{
	unsigned int return_value = 0;

#if defined(CONFIG_PMIC_HW_ACCESS_EN)
	unsigned int pmic_reg = 0;
	unsigned int rdata;

	if ((pmic_suspend_state == true) && irqs_disabled())
		return pmic_config_interface_nolock(RegNum, val, MASK, SHIFT);

	mutex_lock(&pmic_access_mutex);

	/*1. mt_read_byte(RegNum, &pmic_reg);*/
	return_value = pwrap_wacs2(0, (RegNum), 0, &rdata);
	pmic_reg = rdata;
	if (return_value != 0) {
		PMICLOG("[pmic_config_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
		mutex_unlock(&pmic_access_mutex);
		return return_value;
	}
	/*PMICLOG"[pmic_config_interface] Reg[%x]=0x%x\n", RegNum, pmic_reg);*/

	pmic_reg &= ~(MASK << SHIFT);
	pmic_reg |= (val << SHIFT);

	/*2. mt_write_byte(RegNum, pmic_reg);*/
	pmic_config_interface_buck_vsleep_check(RegNum, val, MASK, SHIFT);
	return_value = pwrap_wacs2(1, (RegNum), pmic_reg, &rdata);
	if (return_value != 0) {
		PMICLOG("[pmic_config_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
		mutex_unlock(&pmic_access_mutex);
		return return_value;
	}
	/*PMICLOG"[pmic_config_interface] write Reg[%x]=0x%x\n", RegNum, pmic_reg);*/

#if 0
	/*3. Double Check*/
	/*mt_read_byte(RegNum, &pmic_reg);*/
	return_value = pwrap_wacs2(0, (RegNum), 0, &rdata);
	pmic_reg = rdata;
	if (return_value != 0) {
		PMICLOG("[pmic_config_interface] Reg[%x]= pmic_wrap write data fail\n", RegNum);
		mutex_unlock(&pmic_access_mutex);
		return return_value;
	}
	PMICLOG("[pmic_config_interface] Reg[%x]=0x%x\n", RegNum, pmic_reg);
#endif

	mutex_unlock(&pmic_access_mutex);
#else
	/*PMICLOG("[pmic_config_interface] Can not access HW PMIC\n");*/
#endif

	return return_value;
}

unsigned int pmic_read_interface_nolock(unsigned int RegNum, unsigned int *val, unsigned int MASK, unsigned int SHIFT)
{
	unsigned int return_value = 0;

#if defined(CONFIG_PMIC_HW_ACCESS_EN)
	unsigned int pmic_reg = 0;
	unsigned int rdata;


	/*mt_read_byte(RegNum, &pmic_reg); */
	return_value = pwrap_wacs2(0, (RegNum), 0, &rdata);
	pmic_reg = rdata;
	if (return_value != 0) {
		PMICLOG("[pmic_read_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
		mutex_unlock(&pmic_access_mutex);
		return return_value;
	}
	/*PMICLOG"[pmic_read_interface] Reg[%x]=0x%x\n", RegNum, pmic_reg); */

	pmic_reg &= (MASK << SHIFT);
	*val = (pmic_reg >> SHIFT);
	/*PMICLOG"[pmic_read_interface] val=0x%x\n", *val); */

#else
	/*PMICLOG("[pmic_read_interface] Can not access HW PMIC\n"); */
#endif

	return return_value;
}

unsigned int pmic_config_interface_nolock(unsigned int RegNum, unsigned int val, unsigned int MASK, unsigned int SHIFT)
{
	unsigned int return_value = 0;

#if defined(CONFIG_PMIC_HW_ACCESS_EN)
	unsigned int pmic_reg = 0;
	unsigned int rdata;

    /* pmic wrapper has spinlock protection. pmic do not to do it again */

	/*1. mt_read_byte(RegNum, &pmic_reg); */
	return_value = pwrap_wacs2(0, (RegNum), 0, &rdata);
	pmic_reg = rdata;
	if (return_value != 0) {
		PMICLOG("[pmic_config_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
		mutex_unlock(&pmic_access_mutex);
		return return_value;
	}
	/*PMICLOG"[pmic_config_interface] Reg[%x]=0x%x\n", RegNum, pmic_reg); */

	pmic_reg &= ~(MASK << SHIFT);
	pmic_reg |= (val << SHIFT);

	/*2. mt_write_byte(RegNum, pmic_reg); */
	pmic_config_interface_buck_vsleep_check(RegNum, val, MASK, SHIFT);
	return_value = pwrap_wacs2(1, (RegNum), pmic_reg, &rdata);
	if (return_value != 0) {
		PMICLOG("[pmic_config_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
		mutex_unlock(&pmic_access_mutex);
		return return_value;
	}
	/*PMICLOG"[pmic_config_interface] write Reg[%x]=0x%x\n", RegNum, pmic_reg); */

#if 0
	/*3. Double Check */
	/*mt_read_byte(RegNum, &pmic_reg); */
	return_value = pwrap_wacs2(0, (RegNum), 0, &rdata);
	pmic_reg = rdata;
	if (return_value != 0) {
		PMICLOG("[pmic_config_interface] Reg[%x]= pmic_wrap write data fail\n", RegNum);
		mutex_unlock(&pmic_access_mutex);
		return return_value;
	}
	PMICLOG("[pmic_config_interface] Reg[%x]=0x%x\n", RegNum, pmic_reg);
#endif

#else
	/*PMICLOG("[pmic_config_interface] Can not access HW PMIC\n"); */
#endif

	return return_value;
}

/*****************************************************************************
 * PMIC lock/unlock APIs
 ******************************************************************************/
void pmic_lock(void)
{
	mutex_lock(&pmic_lock_mutex);
}

void pmic_unlock(void)
{
	mutex_unlock(&pmic_lock_mutex);
}

unsigned int upmu_get_reg_value(unsigned int reg)
{
	unsigned int ret = 0;
	unsigned int reg_val = 0;

	ret = pmic_read_interface(reg, &reg_val, 0xFFFF, 0x0);

	return reg_val;
}
EXPORT_SYMBOL(upmu_get_reg_value);

void upmu_set_reg_value(unsigned int reg, unsigned int reg_val)
{
	unsigned int ret = 0;

	ret = pmic_config_interface(reg, reg_val, 0xFFFF, 0x0);
}

unsigned int get_pmic_mt6325_cid(void)
{
	return 0;
}

unsigned int get_mt6325_pmic_chip_version(void)
{
	return 0;
}

/**********************************************************
  *
  *   [Internal Function]
  *
  *********************************************************/
void mt6351_dump_register(void)
{
	unsigned int i = 0;

	PMICLOG("dump PMIC 6351 register\n");

	for (i = 0; i <= 0x0fae; i = i + 10) {
		pr_debug
		    ("Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x\n",
		     i, upmu_get_reg_value(i), i + 1, upmu_get_reg_value(i + 1), i + 2,
		     upmu_get_reg_value(i + 2), i + 3, upmu_get_reg_value(i + 3), i + 4,
		     upmu_get_reg_value(i + 4));

		pr_debug
		    ("Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x\n",
		     i + 5, upmu_get_reg_value(i + 5), i + 6, upmu_get_reg_value(i + 6), i + 7,
		     upmu_get_reg_value(i + 7), i + 8, upmu_get_reg_value(i + 8), i + 9,
		     upmu_get_reg_value(i + 9));
	}

}

/*****************************************************************************
 * workaround for vio18 drop issue in E1
 ******************************************************************************/

static const unsigned char mt6351_VIO[] = {
	12, 13, 14, 15, 0, 1, 2, 3, 8, 8, 8, 8, 8, 9, 10, 11
};

static unsigned char vio18_cal;

void upmu_set_rg_vio18_cal(unsigned int en)
{
#if defined MT6328
	unsigned int chip_version = 0;

	chip_version = pmic_get_register_value(PMIC_SWCID);

	if (chip_version == PMIC6351_E1_CID_CODE) {
		if (en == 1)
			pmic_set_register_value(PMIC_RG_VIO18_CAL, mt6351_VIO[vio18_cal]);
		else
			pmic_set_register_value(PMIC_RG_VIO18_CAL, vio18_cal);
	}

#endif
}
EXPORT_SYMBOL(upmu_set_rg_vio18_cal);


static const unsigned char mt6351_VIO_1_84[] = {
	14, 15, 0, 1, 2, 3, 4, 5, 8, 8, 8, 9, 10, 11, 12, 13
};



void upmu_set_rg_vio18_184(void)
{
	PMICLOG("[upmu_set_rg_vio18_184] old cal=%d new cal=%d.\r\n", vio18_cal,
		mt6351_VIO_1_84[vio18_cal]);
	pmic_set_register_value(PMIC_RG_VIO18_CAL, mt6351_VIO_1_84[vio18_cal]);

}


static const unsigned char mt6351_VMC_1_86[] = {
	14, 15, 0, 1, 2, 3, 4, 5, 8, 8, 8, 9, 10, 11, 12, 13
};


static unsigned char vmc_cal;

void upmu_set_rg_vmc_184(unsigned char x)
{

	PMICLOG("[upmu_set_rg_vio18_184] old cal=%d new cal=%d.\r\n", vmc_cal,
		mt6351_VMC_1_86[vmc_cal]);
	if (x == 0) {
		pmic_set_register_value(PMIC_RG_VMC_CAL, vmc_cal);
		PMICLOG("[upmu_set_rg_vio18_184]:0 old cal=%d new cal=%d.\r\n", vmc_cal, vmc_cal);
	} else {
		pmic_set_register_value(PMIC_RG_VMC_CAL, mt6351_VMC_1_86[vmc_cal]);
		PMICLOG("[upmu_set_rg_vio18_184]:1 old cal=%d new cal=%d.\r\n", vmc_cal,
			mt6351_VMC_1_86[vmc_cal]);
	}
}


static unsigned char vcamd_cal;

static const unsigned char mt6351_vcamd[] = {
	1, 2, 3, 4, 5, 6, 7, 7, 9, 10, 11, 12, 13, 14, 15, 0
};


void upmu_set_rg_vcamd(unsigned char x)
{

	PMICLOG("[upmu_set_rg_vcamd] old cal=%d new cal=%d.\r\n", vcamd_cal,
		mt6351_vcamd[vcamd_cal]);
	if (x == 0) {
		pmic_set_register_value(PMIC_RG_VCAMD_CAL, vcamd_cal);
		PMICLOG("[upmu_set_rg_vcamd]:0 old cal=%d new cal=%d.\r\n", vcamd_cal, vcamd_cal);
	} else {
		pmic_set_register_value(PMIC_RG_VCAMD_CAL, mt6351_vcamd[vcamd_cal]);
		PMICLOG("[upmu_set_rg_vcamd]:1 old cal=%d new cal=%d.\r\n", vcamd_cal,
			mt6351_vcamd[vcamd_cal]);
	}
}




/*****************************************************************************
 * workaround for VMC voltage list
 ******************************************************************************/
int pmic_read_VMC_efuse(void)
{
	unsigned int val;

	pmic_set_register_value(PMIC_RG_EFUSE_CK_PDN_HWEN, 0);
	pmic_set_register_value(PMIC_RG_EFUSE_CK_PDN, 0);
	pmic_set_register_value(PMIC_RG_OTP_RD_SW, 1);

	pmic_set_register_value(PMIC_RG_OTP_PA, 0xd * 2);
	udelay(100);
	if (pmic_get_register_value(PMIC_RG_OTP_RD_TRIG) == 0)
		pmic_set_register_value(PMIC_RG_OTP_RD_TRIG, 1);
	else
		pmic_set_register_value(PMIC_RG_OTP_RD_TRIG, 0);
	udelay(300);
	while (pmic_get_register_value(PMIC_RG_OTP_RD_BUSY) == 1)
		;
	udelay(1000);
	val = pmic_get_register_value(PMIC_RG_OTP_DOUT_SW);

	val = val * 0x80;
	pmic_set_register_value(PMIC_RG_EFUSE_CK_PDN_HWEN, 1);
	pmic_set_register_value(PMIC_RG_EFUSE_CK_PDN, 1);

	return val;
}

/*****************************************************************************
 * upmu_interrupt_chrdet_int_en
 ******************************************************************************/
void upmu_interrupt_chrdet_int_en(unsigned int val)
{
	PMICLOG("[upmu_interrupt_chrdet_int_en] val=%d.\r\n", val);

	/*mt6325_upmu_set_rg_int_en_chrdet(val); */
	pmic_set_register_value(PMIC_RG_INT_EN_CHRDET, val);
}
EXPORT_SYMBOL(upmu_interrupt_chrdet_int_en);


/*****************************************************************************
 * PMIC charger detection
 ******************************************************************************/
unsigned int upmu_get_rgs_chrdet(void)
{
	unsigned int val = 0;

	/*val = mt6325_upmu_get_rgs_chrdet();*/
	val = pmic_get_register_value(PMIC_RGS_CHRDET);
	PMICLOG("[upmu_get_rgs_chrdet] CHRDET status = %d\n", val);

	return val;
}

/*****************************************************************************
 * mt-pmic dev_attr APIs
 ******************************************************************************/
unsigned int g_reg_value = 0;
static ssize_t show_pmic_access(struct device *dev, struct device_attribute *attr, char *buf)
{
	pr_err("[show_pmic_access] 0x%x\n", g_reg_value);
	return sprintf(buf, "%u\n", g_reg_value);
}

static ssize_t store_pmic_access(struct device *dev, struct device_attribute *attr, const char *buf,
				 size_t size)
{
	int ret = 0;
	char *pvalue = NULL, *addr, *val;
	unsigned int reg_value = 0;
	unsigned int reg_address = 0;

	pr_err("[store_pmic_access]\n");
	if (buf != NULL && size != 0) {
		pr_err("[store_pmic_access] buf is %s\n", buf);
		/*reg_address = simple_strtoul(buf, &pvalue, 16);*/

		pvalue = (char *)buf;
		if (size > 5) {
			addr = strsep(&pvalue, " ");
			ret = kstrtou32(addr, 16, (unsigned int *)&reg_address);
		} else
			ret = kstrtou32(pvalue, 16, (unsigned int *)&reg_address);

		if (size > 5) {
			/*reg_value = simple_strtoul((pvalue + 1), NULL, 16);*/
			/*pvalue = (char *)buf + 1;*/
			val =  strsep(&pvalue, " ");
			ret = kstrtou32(val, 16, (unsigned int *)&reg_value);

			pr_err("[store_pmic_access] write PMU reg 0x%x with value 0x%x !\n",
				reg_address, reg_value);
			ret = pmic_config_interface(reg_address, reg_value, 0xFFFF, 0x0);
		} else {
			ret = pmic_read_interface(reg_address, &g_reg_value, 0xFFFF, 0x0);
			pr_err("[store_pmic_access] read PMU reg 0x%x with value 0x%x !\n",
				reg_address, g_reg_value);
			pr_err("[store_pmic_access] use \"cat pmic_access\" to get value(decimal)\r\n");
		}
	}
	return size;
}

static DEVICE_ATTR(pmic_access, 0664, show_pmic_access, store_pmic_access);	/*664*/

/*
 * DVT entry
 */
unsigned char g_reg_value_pmic = 0;

static ssize_t show_pmic_dvt(struct device *dev, struct device_attribute *attr, char *buf)
{
	pr_err("[show_pmic_dvt] 0x%x\n", g_reg_value_pmic);
	return sprintf(buf, "%u\n", g_reg_value_pmic);
}

static ssize_t store_pmic_dvt(struct device *dev, struct device_attribute *attr, const char *buf,
			      size_t size)
{
	int ret = 0;
	char *pvalue = NULL;
	unsigned int test_item = 0;

	pr_err("[store_pmic_dvt]\n");

	if (buf != NULL && size != 0) {
		pr_err("[store_pmic_dvt] buf is %s and size is %zu\n", buf, size);

		/*test_item = simple_strtoul(buf, &pvalue, 10);*/
		pvalue = (char *)buf;
		ret = kstrtou32(pvalue, 16, (unsigned int *)&test_item);
		pr_err("[store_pmic_dvt] test_item=%d\n", test_item);

#ifdef MTK_PMIC_DVT_SUPPORT
		pmic_dvt_entry(test_item);
#else
		pr_err("[store_pmic_dvt] no define MTK_PMIC_DVT_SUPPORT\n");
#endif
	}
	return size;
}

static DEVICE_ATTR(pmic_dvt, 0664, show_pmic_dvt, store_pmic_dvt);

/*
 * auxadc
 */
unsigned char g_auxadc_pmic = 0;

static ssize_t show_pmic_auxadc(struct device *dev, struct device_attribute *attr, char *buf)
{
	pr_err("[show_pmic_auxadc] 0x%x\n", g_auxadc_pmic);
	return sprintf(buf, "%u\n", g_auxadc_pmic);
}

static ssize_t store_pmic_auxadc(struct device *dev, struct device_attribute *attr, const char *buf,
			      size_t size)
{
	int ret = 0, i, j;
	char *pvalue = NULL;
	unsigned int val = 0;

	pr_err("[store_pmic_auxadc]\n");

	if (buf != NULL && size != 0) {
		pvalue = (char *)buf;
		ret = kstrtou32(pvalue, 16, (unsigned int *)&val);
		for (i = 0; i < val; i++) {

			for (j = 0; j < 16; j++) {
				pr_err("[PMIC_AUXADC] [%d]=%d\n", j, PMIC_IMM_GetOneChannelValue(j, 0, 0));
				mdelay(5);
			}
			pr_err("[PMIC_AUXADC] [%d]=%d\n", j, PMIC_IMM_GetOneChannelValue(PMIC_AUX_CH4_DCXO, 0, 0));
		}
	}
	return size;
}

static DEVICE_ATTR(pmic_auxadc_ut, 0664, show_pmic_auxadc, store_pmic_auxadc);

/*****************************************************************************
 * PMIC6351 linux reguplator driver
 ******************************************************************************/

static int mtk_regulator_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	unsigned int add = 0, val = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);
	if (mreg->en_reg != 0) {
		pmic_set_register_value(mreg->en_reg, 1);
		add = pmu_flags_table[mreg->en_reg].offset;
		val = upmu_get_reg_value(add);
	}

	PMICLOG("regulator_enable(name=%s id=%d en_reg=%x vol_reg=%x) [%x]=0x%x\n", rdesc->name,
		rdesc->id, mreg->en_reg, mreg->vol_reg, add, val);

	return 0;
}

static int mtk_regulator_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	unsigned int add = 0, val = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	if (rdev->use_count == 0) {
		PMICLOG("regulator name=%s should not disable( use_count=%d)\n", rdesc->name,
			rdev->use_count);
		return -1;
	}

	if (mreg->en_reg != 0) {
		pmic_set_register_value(mreg->en_reg, 0);
		add = pmu_flags_table[mreg->en_reg].offset;
		val = upmu_get_reg_value(add);
	}

	PMICLOG("regulator_disable(name=%s id=%d en_reg=%x vol_reg=%x use_count=%d) [%x]=0x%x\n",
		rdesc->name, rdesc->id, mreg->en_reg, mreg->vol_reg, rdev->use_count, add, val);

	return 0;
}

static int mtk_regulator_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int en;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	en = pmic_get_register_value(mreg->en_reg);

	PMICLOG("[PMIC]regulator_is_enabled(name=%s id=%d en_reg=%x vol_reg=%x en=%d)\n",
		rdesc->name, rdesc->id, mreg->en_reg, mreg->vol_reg, en);

	return en;
}

static int mtk_regulator_get_voltage_sel(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	unsigned char regVal = 0;
	const int *pVoltage;
	int voltage = 0;
	unsigned int add = 0, val = 9;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	if (mreg->desc.n_voltages != 1) {
		if (mreg->vol_reg != 0) {
			regVal = pmic_get_register_value(mreg->vol_reg);
			if (mreg->pvoltages != NULL) {
				pVoltage = (const int *)mreg->pvoltages;
				/*HW LDO sequence issue, we need to change it */
				if ((strcmp(mreg->desc.name, "va18") == 0) |
					(strcmp(mreg->desc.name, "vtcxo24") == 0) |
					(strcmp(mreg->desc.name, "vtcxo28") == 0) |
					(strcmp(mreg->desc.name, "vcn28") == 0) |
					(strcmp(mreg->desc.name, "vxo22") == 0) |
					(strcmp(mreg->desc.name, "vbif28") == 0)) {
					PMICLOG("mtk_regulator_get_voltage_sel(name=%s selector=%d)\n",
						mreg->desc.name, regVal);
					if (regVal == 0)
						regVal = 3;
					else if  (regVal == 1)
						regVal = 2;
					else if  (regVal == 2)
						regVal = 1;
					else if  (regVal == 3)
						regVal = 0;
				}
				voltage = pVoltage[regVal];
				add = pmu_flags_table[mreg->en_reg].offset;
				val = upmu_get_reg_value(add);
			} else {
				voltage = mreg->desc.min_uV + mreg->desc.uV_step * regVal;
			}
		} else {
			PMICLOG
			    ("regulator_get_voltage_sel bugl(name=%s id=%d en_reg=%x vol_reg=%x)\n",
			     rdesc->name, rdesc->id, mreg->en_reg, mreg->vol_reg);
		}
	} else {
		if (mreg->vol_reg != 0) {
			regVal = 0;
			pVoltage = (const int *)mreg->pvoltages;
			voltage = pVoltage[regVal];
		} else {
			PMICLOG
			    ("regulator_get_voltage_sel bugl(name=%s id=%d en_reg=%x vol_reg=%x)\n",
			     rdesc->name, rdesc->id, mreg->en_reg, mreg->vol_reg);
		}
	}
	/*HW LDO sequence issue, we need to change it */
	if ((strcmp(mreg->desc.name, "va18") == 0) |
		(strcmp(mreg->desc.name, "vtcxo24") == 0) |
		(strcmp(mreg->desc.name, "vtcxo28") == 0) |
		(strcmp(mreg->desc.name, "vcn28") == 0) |
		(strcmp(mreg->desc.name, "vxo22") == 0) |
		(strcmp(mreg->desc.name, "vbif28") == 0)) {
		PMICLOG("mtk_regulator_get_voltage_sel_restore(name=%s selector=%d)\n",
			mreg->desc.name, regVal);
			if (regVal == 0)
				regVal = 3;
			else if  (regVal == 1)
				regVal = 2;
			else if  (regVal == 2)
				regVal = 1;
			else if  (regVal == 3)
				regVal = 0;
	}
	PMICLOG
	    ("regulator_get_voltage_sel(name=%s id=%d en_reg=%x vol_reg=%x reg/sel:%d voltage:%d [0x%x]=0x%x)\n",
	     rdesc->name, rdesc->id, mreg->en_reg, mreg->vol_reg, regVal, voltage, add, val);

	return regVal;
}

static int mtk_regulator_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	mreg->vosel.cur_sel = selector;

	PMICLOG("regulator_set_voltage_sel(name=%s id=%d en_reg=%x vol_reg=%x selector=%d)\n",
		rdesc->name, rdesc->id, mreg->en_reg, mreg->vol_reg, selector);

#if 0
	if (strcmp(rdesc->name, "VCAMD") == 0) {

		if (selector == 3)
			upmu_set_rg_vcamd(1);
		else
			upmu_set_rg_vcamd(0);
	}
#endif
	/*HW LDO sequence issue, we need to change it */
/*
	if ((strcmp(mreg->desc.name, "va18") == 0) |
		(strcmp(mreg->desc.name, "vtcxo24") == 0) |
		(strcmp(mreg->desc.name, "vtcxo28") == 0) |
		(strcmp(mreg->desc.name, "vcn28") == 0) |
		(strcmp(mreg->desc.name, "vxo22") == 0) |
		(strcmp(mreg->desc.name, "vbif28") == 0)) {
		PMICLOG("regulator_set_voltage_sel (name=%s selector=%d)\n",
			rdesc->name, selector);
			if (selector == 0)
				selector = 3;
			else if  (selector == 1)
				selector = 2;
			else if  (selector == 2)
				selector = 1;
			else if  (selector == 3)
				selector = 0;
	}
*/
	pr_err("regulator_set_voltage_sel(name=%s selector=%d)\n",
		rdesc->name, selector);
	if (mreg->vol_reg != 0)
		pmic_set_register_value(mreg->vol_reg, selector);

	return 0;
}

static int mtk_regulator_list_voltage(struct regulator_dev *rdev, unsigned selector)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	const int *pVoltage;
	int voltage = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	if (mreg->desc.n_voltages != 1) {
		if (mreg->vol_reg != 0) {
			if (mreg->pvoltages != NULL) {
				pVoltage = (const int *)mreg->pvoltages;
				/*HW LDO sequence issue, we need to change it */
				if ((strcmp(mreg->desc.name, "va18") == 0) |
					(strcmp(mreg->desc.name, "vtcxo24") == 0) |
					(strcmp(mreg->desc.name, "vtcxo28") == 0) |
					(strcmp(mreg->desc.name, "vcn28") == 0) |
					(strcmp(mreg->desc.name, "vxo22") == 0) |
					(strcmp(mreg->desc.name, "vbif28") == 0)) {
					PMICLOG("mtk_regulator_list_voltage(name=%s selector=%d)\n",
						mreg->desc.name, selector);
					if (selector == 0)
						selector = 3;
					else if  (selector == 1)
						selector = 2;
					else if  (selector == 2)
						selector = 1;
					else if  (selector == 3)
						selector = 0;
				}
				voltage = pVoltage[selector];
			} else {
				voltage = mreg->desc.min_uV + mreg->desc.uV_step * selector;
			}
		} else {
			PMICLOG
			    ("mtk_regulator_list_voltage bugl(name=%s id=%d en_reg=%x vol_reg=%x)\n",
			     rdesc->name, rdesc->id, mreg->en_reg, mreg->vol_reg);
		}
	} else {
		pVoltage = (const int *)mreg->pvoltages;
		voltage = pVoltage[0];
	}
	return voltage;
}




static struct regulator_ops mtk_regulator_ops = {
	.enable = mtk_regulator_enable,
	.disable = mtk_regulator_disable,
	.is_enabled = mtk_regulator_is_enabled,
	.get_voltage_sel = mtk_regulator_get_voltage_sel,
	.set_voltage_sel = mtk_regulator_set_voltage_sel,
	.list_voltage = mtk_regulator_list_voltage,
};

static ssize_t show_LDO_STATUS(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_regulator *mreg;
	unsigned int ret_value = 0;

	mreg = container_of(attr, struct mtk_regulator, en_att);

	ret_value = pmic_get_register_value(mreg->en_reg);

	pr_debug("[EM] LDO_%s_STATUS : %d\n", mreg->desc.name, ret_value);
	return sprintf(buf, "%u\n", ret_value);
}

static ssize_t store_LDO_STATUS(struct device *dev, struct device_attribute *attr, const char *buf,
				size_t size)
{
	PMICLOG("[EM] Not Support Write Function\n");
	return size;
}

static ssize_t show_LDO_VOLTAGE(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_regulator *mreg;
	const int *pVoltage;

	unsigned short regVal;
	unsigned int ret_value = 0;

	mreg = container_of(attr, struct mtk_regulator, voltage_att);

	if (mreg->desc.n_voltages != 1) {
		if (mreg->vol_reg != 0) {
			regVal = pmic_get_register_value(mreg->vol_reg);
			if (mreg->pvoltages != NULL) {
				pVoltage = (const int *)mreg->pvoltages;
				/*HW LDO sequence issue, we need to change it */
				if ((strcmp(mreg->desc.name, "va18") == 0) |
					(strcmp(mreg->desc.name, "vtcxo24") == 0) |
					(strcmp(mreg->desc.name, "vtcxo28") == 0) |
					(strcmp(mreg->desc.name, "vcn28") == 0) |
					(strcmp(mreg->desc.name, "vxo22") == 0) |
					(strcmp(mreg->desc.name, "vbif28") == 0)) {
					PMICLOG("show_LDO_VOLTAGE(name=%s selector=%d)\n",
						mreg->desc.name, regVal);
					if (regVal == 0)
						regVal = 3;
					else if  (regVal == 1)
						regVal = 2;
					else if  (regVal == 2)
						regVal = 1;
					else if  (regVal == 3)
						regVal = 0;
				}
				ret_value = pVoltage[regVal];
			} else {
				ret_value = mreg->desc.min_uV + mreg->desc.uV_step * regVal;
			}
		} else {
			PMICLOG("[EM][ERROR] LDO_%s_VOLTAGE : voltage=0 vol_reg=0\n",
				mreg->desc.name);
		}
	} else {
		pVoltage = (const int *)mreg->pvoltages;
		ret_value = pVoltage[0];
	}

	ret_value = ret_value / 1000;
	pr_err("[EM] LDO_%s_VOLTAGE : %d\n", mreg->desc.name, ret_value);
	return sprintf(buf, "%u\n", ret_value);

}

static ssize_t store_LDO_VOLTAGE(struct device *dev, struct device_attribute *attr, const char *buf,
				 size_t size)
{
	PMICLOG("[EM] Not Support Write Function\n");
	return size;
}

static ssize_t show_BUCK_STATUS(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_regulator *mreg;
	unsigned int ret_value = 0;

	mreg = container_of(attr, struct mtk_regulator, en_att);

	ret_value = pmic_get_register_value(mreg->qi_en_reg);

	pr_debug("[EM] BUCK_%s_STATUS : %d\n", mreg->desc.name, ret_value);
	return sprintf(buf, "%u\n", ret_value);
}

static ssize_t store_BUCK_STATUS(struct device *dev, struct device_attribute *attr, const char *buf,
				 size_t size)
{
	PMICLOG("[EM] Not Support Write Function\n");
	return size;
}

static ssize_t show_BUCK_VOLTAGE(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mtk_regulator *mreg;
	const int *pVoltage;

	unsigned short regVal;
	unsigned int ret_value = 0;

	mreg = container_of(attr, struct mtk_regulator, voltage_att);

	if (mreg->desc.n_voltages != 1) {
		if (mreg->qi_vol_reg != 0) {
			regVal = pmic_get_register_value(mreg->qi_vol_reg);
			if (mreg->pvoltages != NULL) {
				pVoltage = (const int *)mreg->pvoltages;
				ret_value = pVoltage[regVal];
			} else {
				ret_value = mreg->desc.min_uV + mreg->desc.uV_step * regVal;
			}
		} else {
			pr_debug("[EM][ERROR] buck_%s_VOLTAGE : voltage=0 vol_reg=0\n",
				mreg->desc.name);
		}
	} else {
		pVoltage = (const int *)mreg->pvoltages;
		ret_value = pVoltage[0];
	}

	ret_value = ret_value / 1000;
	pr_debug("[EM] BUCK_%s_VOLTAGE : %d\n", mreg->desc.name, ret_value);
	return sprintf(buf, "%u\n", ret_value);
}

static ssize_t store_BUCK_VOLTAGE(struct device *dev, struct device_attribute *attr,
				  const char *buf, size_t size)
{
	PMICLOG("[EM] Not Support Write Function\n");
	return size;
}

/*LDO setting*/
static const int mt6351_VA18_voltages[] = {
	1800000,
	2200000,
	2375000,
	2800000,
};
static const int mt6351_VCAMA_voltages[] = {
	1500000,
	1800000,
	2500000,
	2800000,
};

static const int mt6351_VCN33_voltages[] = {
	3300000,
	3400000,
	3500000,
	3600000,
};

static const int mt6351_VEFUSE_voltages[] = {
	1200000,
	1300000,
	1700000,
	1800000,
	1860000,
	2760000,
	3000000,
	3100000,
};

/*VSIM1 & VSIM2*/
static const int mt6351_VSIM1_voltages[] = {
	1200000,
	1300000,
	1700000,
	1800000,
	1860000,
	2760000,
	3000000,
	3100000,
};

static const int mt6351_VEMC33_voltages[] = {
	3000000,
	3300000,
};

static const int mt6351_VMCH_voltages[] = {
	3000000,
	3300000,
};

static const int mt6351_VMC_voltages[] = {
	1200000,
	1300000,
	1500000,
	1800000,
	2000000,
	2900000,
	3000000,
	3300000,
};

static const int mt6351_VMC_E1_1_voltages[] = {
	2500000,
	2900000,
	3000000,
	3300000,
};

static const int mt6351_VMC_E1_2_voltages[] = {
	1300000,
	1800000,
	2900000,
	3300000,
};


/*VCAM_AF & VIBR*/
static const int mt6351_VCAM_AF_voltages[] = {
	1200000,
	1300000,
	1500000,
	1800000,
	2000000,
	2800000,
	3000000,
	3300000,
};

static const int mt6351_VGP1_voltages[] = {
	1200000,
	1300000,
	1500000,
	1800000,
	2500000,
	2800000,
	3000000,
	3300000,
};

static const int mt6351_VXO22_voltages[] = {
	1800000,
	2200000,
	2375000,
	2800000,
};

static const int mt6351_VCAMD_voltages[] = {
	900000,
	950000,
	1000000,
	1050000,
	1100000,
	1200000,
	1210000,
};

static const int mt6351_VA10_voltages[] = {
	900000,
	950000,
	1000000,
	1050000,
	1200000,
	1500000,
	1800000,
};

static const int mt6351_VRF18_1_voltages[] = {
	1000000,
	1200000,
	1300000,
	1500000,
	1800000,
	1810000,
};

static const int mt6351_VGP3_voltages[] = {
	1000000,
	1050000,
	1100000,
	1220000,
	1300000,
	1500000,
	1800000,
	1810000,
};

static const int mt6351_VCAM_IO_voltages[] = {
	900000,
	950000,
	1000000,
	1050000,
	1200000,
	1500000,
	1800000,
};

static const int mt6351_VM_voltages[] = {
	1800000,
	2900000,
	3000000,
	3300000,
};

static const int mt6351_1v8_voltages[] = {
	1800000,
};

static const int mt6351_2v8_voltages[] = {
	2800000,
};

static const int mt6351_3v3_voltages[] = {
	3300000,
};

static const int mt6351_1v825_voltages[] = {
	1825000,
};

static const int mt6351_rf18_voltages[] = {
	1000000,
	1050000,
	1100000,
	1220000,
	1300000,
	1500000,
	1800000,
	1810000,
};

struct mtk_regulator mtk_ldos[] = {
	PMIC_LDO_GEN1(va18, PMIC_RG_VA18_EN, PMIC_RG_VA18_VOSEL,
	mt6351_VA18_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vtcxo24, PMIC_RG_VTCXO24_EN, PMIC_RG_VTCXO24_VOSEL,
	mt6351_VA18_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vtcxo28, PMIC_RG_VTCXO28_EN,
	PMIC_RG_VTCXO28_VOSEL, mt6351_VA18_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vcn28, PMIC_RG_VCN28_EN, PMIC_RG_VCN28_VOSEL,
	mt6351_VA18_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vcama, PMIC_RG_VCAMA_EN, PMIC_RG_VCAMA_VOSEL,
		      mt6351_VCAMA_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vusb33, PMIC_RG_VUSB33_EN, NULL, mt6351_3v3_voltages, 1, PMIC_EN),
	PMIC_LDO_GEN1(vsim1, PMIC_RG_VSIM1_EN, PMIC_RG_VSIM1_VOSEL,
		      mt6351_VSIM1_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vsim2, PMIC_RG_VSIM2_EN, PMIC_RG_VSIM2_VOSEL,
		      mt6351_VSIM1_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vemc, PMIC_RG_VEMC_EN, PMIC_RG_VEMC_VOSEL,
		      mt6351_VEMC33_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vmch, PMIC_RG_VMCH_EN, PMIC_RG_VMCH_VOSEL,
		      mt6351_VMCH_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vio28, PMIC_RG_VIO28_EN, NULL, mt6351_2v8_voltages, 1, PMIC_EN),
	PMIC_LDO_GEN1(vibr, PMIC_RG_VIBR_EN, PMIC_RG_VIBR_VOSEL,
		      mt6351_VCAM_AF_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vcamd, PMIC_RG_VCAMD_EN, PMIC_RG_VCAMD_VOSEL,
		      mt6351_VCAMD_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vrf18, PMIC_RG_VRF18_EN, PMIC_RG_VRF18_VOSEL,
	mt6351_rf18_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vio18, PMIC_RG_VIO18_EN, NULL, mt6351_1v8_voltages, 1, PMIC_EN),
	PMIC_LDO_GEN1(vcn18, PMIC_RG_VCN18_EN, PMIC_RG_VCN18_VOSEL,
	mt6351_VCAM_IO_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vcamio, PMIC_RG_VCAMIO_EN, PMIC_RG_VCAMIO_VOSEL,
		      mt6351_VCAM_IO_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vsram_proc, PMIC_RG_VSRAM_PROC_EN, PMIC_BUCK_VSRAM_PROC_VOSEL,
		      mt6351_2v8_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vxo22, PMIC_RG_VXO22_EN, PMIC_RG_VXO22_VOSEL,
		      mt6351_VA18_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vrf12, PMIC_RG_VRF12_EN, PMIC_RG_VRF12_VOSEL,
		      mt6351_VCAM_IO_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(va10, PMIC_RG_VA10_EN, PMIC_RG_VA10_VOSEL,
		      mt6351_VA10_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vdram, PMIC_RG_VDRAM_EN, PMIC_RG_VDRAM_VOSEL,
		      mt6351_VCAMD_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vmipi, PMIC_RG_VMIPI_EN, PMIC_RG_VMIPI_VOSEL,
		      mt6351_VA10_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vgp3, PMIC_RG_VGP3_EN, PMIC_RG_VGP3_VOSEL,
		      mt6351_VGP3_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vbif28, PMIC_RG_VBIF28_EN, PMIC_RG_VBIF28_VOSEL,
		      mt6351_VA18_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vefuse, PMIC_RG_VEFUSE_EN, PMIC_RG_VEFUSE_VOSEL,
		      mt6351_VEFUSE_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vcn33_bt, PMIC_RG_VCN33_EN_BT, PMIC_RG_VCN33_VOSEL,
		      mt6351_VCN33_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vcn33_wifi, PMIC_RG_VCN33_EN_WIFI, PMIC_RG_VCN33_VOSEL,
		      mt6351_VCN33_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vldo28, PMIC_RG_VLDO28_EN_0, NULL, mt6351_2v8_voltages, 1, PMIC_EN),
	PMIC_LDO_GEN1(vmc, PMIC_RG_VMC_EN, PMIC_RG_VMC_VOSEL,
	mt6351_VMC_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vldo28_0, PMIC_RG_VLDO28_EN_0, NULL, mt6351_2v8_voltages, 1, PMIC_EN),
	PMIC_LDO_GEN1(vldo28_1, PMIC_RG_VLDO28_EN_1, NULL, mt6351_2v8_voltages, 1, PMIC_EN),
};

static struct mtk_regulator mtk_bucks[] = {
	PMIC_BUCK_GEN(VCORE, PMIC_BUCK_VCORE_EN, PMIC_BUCK_VCORE_VOSEL, 600000,
		      1393750, 6250),
	PMIC_BUCK_GEN(VGPU, PMIC_BUCK_VGPU_EN, PMIC_BUCK_VGPU_VOSEL, 600000,
		      1393750, 6250),
	PMIC_BUCK_GEN(VMODEM, PMIC_BUCK_VMODEM_EN, PMIC_BUCK_VMODEM_VOSEL, 600000,
		      1393750, 6250),
	PMIC_BUCK_GEN(VMD1, PMIC_BUCK_VMD1_EN, PMIC_BUCK_VMD1_VOSEL, 600000,
		      1393750, 6250),
	PMIC_BUCK_GEN(VSRAM_MD, PMIC_BUCK_VSRAM_MD_EN, PMIC_BUCK_VSRAM_MD_VOSEL,
		      600000, 1393750, 6250),
	PMIC_BUCK_GEN(VS1, PMIC_BUCK_VS1_EN, PMIC_BUCK_VS1_VOSEL, 600000, 1393750,
		      6250),
	PMIC_BUCK_GEN(VS2, PMIC_BUCK_VS2_EN, PMIC_BUCK_VS2_VOSEL, 600000, 1393750,
		      6250),
	PMIC_BUCK_GEN(VPA, PMIC_BUCK_VPA_EN, PMIC_BUCK_VPA_VOSEL, 600000, 1393750,
		      6250),
	PMIC_BUCK_GEN(VSRAM_PROC, PMIC_BUCK_VSRAM_PROC_EN,
		      PMIC_BUCK_VSRAM_PROC_VOSEL, 600000, 1393750, 6250),
};

#if !defined CONFIG_MTK_LEGACY
#ifdef CONFIG_OF
/*
#define PMIC_REGULATOR_OF_MATCH(_name, _id)			\
[MT6328_POWER_LDO_##_id] = {						\
	.name = #_name,						\
	.driver_data = &mtk_ldos[MT6328_POWER_LDO_##_id],	\
}
*/
#define PMIC_REGULATOR_OF_MATCH(_name, _id)			\
	{						\
		.name = #_name,						\
		.driver_data = &mtk_ldos[MT6351_POWER_LDO_##_id],	\
	}

static struct of_regulator_match pmic_regulator_matches[] = {
	PMIC_REGULATOR_OF_MATCH(ldo_va18, VA18),
	PMIC_REGULATOR_OF_MATCH(ldo_vtcxo24, VTCXO24),
	PMIC_REGULATOR_OF_MATCH(ldo_vtcxo28, VTCXO28),
	PMIC_REGULATOR_OF_MATCH(ldo_vcn28, VCN28),
	PMIC_REGULATOR_OF_MATCH(ldo_vcama, VCAMA),
	PMIC_REGULATOR_OF_MATCH(ldo_vusb33, VUSB33),
	PMIC_REGULATOR_OF_MATCH(ldo_vsim1, VSIM1),
	PMIC_REGULATOR_OF_MATCH(ldo_vsim2, VSIM2),
	PMIC_REGULATOR_OF_MATCH(ldo_vemc, VEMC),
	PMIC_REGULATOR_OF_MATCH(ldo_vmch, VMCH),
	PMIC_REGULATOR_OF_MATCH(ldo_vio28, VIO28),
	PMIC_REGULATOR_OF_MATCH(ldo_vibr, VIBR),
	PMIC_REGULATOR_OF_MATCH(ldo_vcamd, VCAMD),
	PMIC_REGULATOR_OF_MATCH(ldo_vrf18, VRF18),
	PMIC_REGULATOR_OF_MATCH(ldo_vio18, VIO18),
	PMIC_REGULATOR_OF_MATCH(ldo_vcn18, VCN18),
	PMIC_REGULATOR_OF_MATCH(ldo_vcamio, VCAMIO),
	PMIC_REGULATOR_OF_MATCH(ldo_vsram_proc, VSRAM_PROC),
	PMIC_REGULATOR_OF_MATCH(ldo_vxo22, VXO22),
	PMIC_REGULATOR_OF_MATCH(ldo_vrf12, VRF12),
	PMIC_REGULATOR_OF_MATCH(ldo_va10, VA10),
	PMIC_REGULATOR_OF_MATCH(ldo_vdram, VDRAM),
	PMIC_REGULATOR_OF_MATCH(ldo_vmipi, VMIPI),
	PMIC_REGULATOR_OF_MATCH(ldo_vgp3, VGP3),
	PMIC_REGULATOR_OF_MATCH(ldo_vbif28, VBIF28),
	PMIC_REGULATOR_OF_MATCH(ldo_vefuse, VEFUSE),
	PMIC_REGULATOR_OF_MATCH(ldo_vcn33_bt, VCN33_BT),
	PMIC_REGULATOR_OF_MATCH(ldo_vcn33_wifi, VCN33_WIFI),
	PMIC_REGULATOR_OF_MATCH(ldo_vldo28, VLDO28),
	PMIC_REGULATOR_OF_MATCH(ldo_vmc, VMC),
	PMIC_REGULATOR_OF_MATCH(ldo_vldo28_0, VLDO28),
	PMIC_REGULATOR_OF_MATCH(ldo_vldo28_1, VLDO28),
};
#endif				/* End of #ifdef CONFIG_OF */
#endif				/* End of #if !defined CONFIG_MTK_LEGACY */

#if !defined CONFIG_MTK_LEGACY
#ifdef CONFIG_OF
struct platform_device mt_pmic_device = {
	.name = "pmic_regulator",
	.id = -1,
};

static const struct platform_device_id pmic_regulator_id[] = {
	{"pmic_regulator", 0},
	{},
};

static const struct of_device_id pmic_cust_of_ids[] = {
	{.compatible = "mediatek,mt6351",},
	{},
};

MODULE_DEVICE_TABLE(of, pmic_cust_of_ids);

static int pmic_regulator_ldo_init(struct platform_device *pdev)
{
	struct device_node *np, *regulators;
	int matched, i = 0, ret;

	pdev->dev.of_node = of_find_compatible_node(NULL, NULL, "mediatek,mt_pmic");
	np = of_node_get(pdev->dev.of_node);
	if (!np)
		return -EINVAL;

	regulators = of_get_child_by_name(np, "ldo_regulators");
	if (!regulators) {
		PMICLOG("[PMIC]regulators node not found\n");
		ret = -EINVAL;
		goto out;
	}
	matched = of_regulator_match(&pdev->dev, regulators,
				     pmic_regulator_matches,
				     ARRAY_SIZE(pmic_regulator_matches));
	if ((matched < 0) || (matched != MT65XX_POWER_COUNT_END)) {
		pr_err("[PMIC]Error parsing regulator init data: %d %d\n", matched,
			MT65XX_POWER_COUNT_END);
		return matched;
	}

	for (i = 0; i < ARRAY_SIZE(pmic_regulator_matches); i++) {
		if (mtk_ldos[i].isUsedable == 1) {
			mtk_ldos[i].config.dev = &(pdev->dev);
			mtk_ldos[i].config.init_data = pmic_regulator_matches[i].init_data;
			mtk_ldos[i].config.of_node = pmic_regulator_matches[i].of_node;
			mtk_ldos[i].config.driver_data = pmic_regulator_matches[i].driver_data;
			mtk_ldos[i].desc.owner = THIS_MODULE;

			mtk_ldos[i].rdev =
			    regulator_register(&mtk_ldos[i].desc, &mtk_ldos[i].config);
			if (IS_ERR(mtk_ldos[i].rdev)) {
				ret = PTR_ERR(mtk_ldos[i].rdev);
				pr_warn("[regulator_register] failed to register %s (%d)\n",
					mtk_ldos[i].desc.name, ret);
			} else {
				PMICLOG("[regulator_register] pass to register %s\n",
					mtk_ldos[i].desc.name);

				mtk_ldos[i].vosel.def_sel = mtk_regulator_get_voltage_sel(mtk_ldos[i].rdev);
				mtk_ldos[i].vosel.cur_sel = mtk_ldos[i].vosel.def_sel;
			}

			PMICLOG("[PMIC]mtk_ldos[%d].config.init_data min_uv:%d max_uv:%d\n", i,
				mtk_ldos[i].config.init_data->constraints.min_uV,
				mtk_ldos[i].config.init_data->constraints.max_uV);
		}
	}
	of_node_put(regulators);
	return 0;

out:
	of_node_put(np);
	return ret;
}

static int pmic_mt_cust_probe(struct platform_device *pdev)
{
	struct device_node *np, *nproot, *regulators, *child;
#ifdef non_ks
	const struct of_device_id *match;
#endif
	int ret;
	unsigned int i = 0, default_on;

	PMICLOG("[PMIC]pmic_mt_cust_probe %s %s\n", pdev->name, pdev->id_entry->name);
#ifdef non_ks
	/* check if device_id is matched */
	match = of_match_device(pmic_cust_of_ids, &pdev->dev);
	if (!match) {
		pr_warn("[PMIC]pmic_cust_of_ids do not matched\n");
		return -EINVAL;
	}

	/* check customer setting */
	nproot = of_find_compatible_node(NULL, NULL, "mediatek,mt6328");
	if (nproot == NULL) {
		pr_info("[PMIC]pmic_mt_cust_probe get node failed\n");
		return -ENOMEM;
	}

	np = of_node_get(nproot);
	if (!np) {
		pr_info("[PMIC]pmic_mt_cust_probe of_node_get fail\n");
		return -ENODEV;
	}

	regulators = of_find_node_by_name(np, "regulators");
	if (!regulators) {
		pr_info("[PMIC]failed to find regulators node\n");
		ret = -ENODEV;
		goto out;
	}
	for_each_child_of_node(regulators, child) {
		/* check ldo regualtors and set it */
		for (i = 0; i < ARRAY_SIZE(pmic_regulator_matches); i++) {
			/* compare dt name & ldos name */
			if (!of_node_cmp(child->name, pmic_regulator_matches[i].name)) {
				PMICLOG("[PMIC]%s regulator_matches %s\n", child->name,
					(char *)of_get_property(child, "regulator-name", NULL));
				break;
			}
		}
		if (i == ARRAY_SIZE(pmic_regulator_matches))
			continue;
		if (!of_property_read_u32(child, "regulator-default-on", &default_on)) {
			switch (default_on) {
			case 0:
				/* skip */
				PMICLOG("[PMIC]%s regulator_skip %s\n", child->name,
					pmic_regulator_matches[i].name);
				break;
			case 1:
				/* turn ldo off */
				pmic_set_register_value(mtk_ldos[i].en_reg, false);
				PMICLOG("[PMIC]%s default is off\n",
					(char *)of_get_property(child, "regulator-name", NULL));
				break;
			case 2:
				/* turn ldo on */
				pmic_set_register_value(mtk_ldos[i].en_reg, true);
				PMICLOG("[PMIC]%s default is on\n",
					(char *)of_get_property(child, "regulator-name", NULL));
				break;
			default:
				break;
			}
		}
	}
	of_node_put(regulators);
	PMICLOG("[PMIC]pmic_mt_cust_probe done\n");
	return 0;
#else
	nproot = of_find_compatible_node(NULL, NULL, "mediatek,mt_pmic");
	if (nproot == NULL) {
		pr_info("[PMIC]pmic_mt_cust_probe get node failed\n");
		return -ENOMEM;
	}

	np = of_node_get(nproot);
	if (!np) {
		pr_info("[PMIC]pmic_mt_cust_probe of_node_get fail\n");
		return -ENODEV;
	}

	regulators = of_get_child_by_name(np, "ldo_regulators");
	if (!regulators) {
		PMICLOG("[PMIC]pmic_mt_cust_probe ldo regulators node not found\n");
		ret = -ENODEV;
		goto out;
	}

	for_each_child_of_node(regulators, child) {
		/* check ldo regualtors and set it */
		if (!of_property_read_u32(child, "regulator-default-on", &default_on)) {
			switch (default_on) {
			case 0:
				/* skip */
				PMICLOG("[PMIC]%s regulator_skip %s\n", child->name,
					pmic_regulator_matches[i].name);
				break;
			case 1:
				/* turn ldo off */
				pmic_set_register_value(mtk_ldos[i].en_reg, false);
				PMICLOG("[PMIC]%s default is off\n",
					(char *)of_get_property(child, "regulator-name", NULL));
				break;
			case 2:
				/* turn ldo on */
				pmic_set_register_value(mtk_ldos[i].en_reg, true);
				PMICLOG("[PMIC]%s default is on\n",
					(char *)of_get_property(child, "regulator-name", NULL));
				break;
			default:
				break;
			}
		}
	}
	of_node_put(regulators);
	pr_err("[PMIC]pmic_mt_cust_probe done\n");
	return 0;
#endif
out:
	of_node_put(np);
	return ret;
}

static int pmic_mt_cust_remove(struct platform_device *pdev)
{
       /*platform_driver_unregister(&mt_pmic_driver_probe);*/
	return 0;
}

static struct platform_driver mt_pmic_driver_probe = {
	.driver = {
		   .name = "pmic_regulator",
		   .owner = THIS_MODULE,
		   .of_match_table = pmic_cust_of_ids,
		   },
	.probe = pmic_mt_cust_probe,
	.remove = pmic_mt_cust_remove,
/*      .id_table = pmic_regulator_id,*/
};
#endif				/* End of #ifdef CONFIG_OF */
#endif				/* End of #if !defined CONFIG_MTK_LEGACY */

void pmic_regulator_suspend(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mtk_ldos); i++) {
			if (mtk_ldos[i].vol_reg != 0) {
				mtk_ldos[i].vosel.cur_sel = mtk_regulator_get_voltage_sel(mtk_ldos[i].rdev);
				if (mtk_ldos[i].vosel.cur_sel != mtk_ldos[i].vosel.def_sel) {
					mtk_ldos[i].vosel.restore = true;
					pr_err("pmic_regulator_suspend(name=%s id=%d default_sel=%d current_sel=%d)\n",
							mtk_ldos[i].rdev->desc->name, mtk_ldos[i].rdev->desc->id,
							mtk_ldos[i].vosel.def_sel, mtk_ldos[i].vosel.cur_sel);
				} else
					mtk_ldos[i].vosel.restore = false;
			}
	}
}

void pmic_regulator_resume(void)
{
	int i, selector;

	for (i = 0; i < ARRAY_SIZE(mtk_ldos); i++) {
			if (mtk_ldos[i].vol_reg != 0) {
				if (mtk_ldos[i].vosel.restore == true) {
					/*-- regulator voltage changed? --*/
					selector = mtk_ldos[i].vosel.cur_sel;
					pmic_set_register_value(mtk_ldos[i].vol_reg, selector);
					pr_err("pmic_regulator_resume(name=%s id=%d default_sel=%d current_sel=%d)\n",
						mtk_ldos[i].rdev->desc->name, mtk_ldos[i].rdev->desc->id,
						mtk_ldos[i].vosel.def_sel, mtk_ldos[i].vosel.cur_sel);
				}
			}
	}
}

static int pmic_regulator_pm_event(struct notifier_block *notifier, unsigned long pm_event, void *unused)
{
	switch (pm_event) {
	case PM_HIBERNATION_PREPARE:	/* Going to hibernate */
		pr_warn("[%s] pm_event %lu (IPOH_Start)\n", __func__, pm_event);
		pmic_regulator_suspend();
		return NOTIFY_DONE;

	case PM_POST_HIBERNATION:	/* Hibernation finished */
		pr_warn("[%s] pm_event %lu (IPOH_End)\n", __func__, pm_event);
		pmic_regulator_resume();
		return NOTIFY_DONE;
	}
	return NOTIFY_OK;
}

static struct notifier_block pmic_regulator_pm_notifier_block = {
	.notifier_call = pmic_regulator_pm_event,
	.priority = 0,
};

void mtk_regulator_init(struct platform_device *dev)
{
#if defined CONFIG_MTK_LEGACY
	int i = 0;
	int isEn = 0;
#endif
	int ret = 0;

	/*workaround for VMC voltage */
	if (pmic_get_register_value(PMIC_SWCID) == PMIC6351_E1_CID_CODE) {
		if (pmic_read_VMC_efuse() != 0) {
			PMICLOG("VMC voltage use E1_2 voltage table\n");
			mtk_ldos[MT6351_POWER_LDO_VMC].pvoltages = (void *)mt6351_VMC_E1_2_voltages;
		} else {
			PMICLOG("VMC voltage use E1_1 voltage table\n");
			mtk_ldos[MT6351_POWER_LDO_VMC].pvoltages = (void *)mt6351_VMC_E1_1_voltages;
		}
	}
/*workaround for VIO18*/
	vio18_cal = pmic_get_register_value(PMIC_RG_VIO18_CAL);
	vmc_cal = pmic_get_register_value(PMIC_RG_VMC_CAL);
	vcamd_cal = pmic_get_register_value(PMIC_RG_VCAMD_CAL);

#if !defined CONFIG_MTK_LEGACY
#ifdef CONFIG_OF
	pmic_regulator_ldo_init(dev);
#endif
#else
	for (i = 0; i < ARRAY_SIZE(mtk_ldos); i++) {
		if (mtk_ldos[i].isUsedable == 1) {
			mtk_ldos[i].config.dev = &(dev->dev);
			mtk_ldos[i].config.init_data = &mtk_ldos[i].init_data;
			if (mtk_ldos[i].desc.n_voltages != 1) {
				const int *pVoltage;

				if (mtk_ldos[i].vol_reg != 0) {
					if (mtk_ldos[i].pvoltages != NULL) {
						pVoltage = (const int *)mtk_ldos[i].pvoltages;
						mtk_ldos[i].init_data.constraints.max_uV =
						    pVoltage[mtk_ldos[i].desc.n_voltages - 1];
						mtk_ldos[i].init_data.constraints.min_uV =
						    pVoltage[0];
					} else {
						mtk_ldos[i].init_data.constraints.max_uV =
						    (mtk_ldos[i].desc.n_voltages -
						     1) * mtk_ldos[i].desc.uV_step +
						    mtk_ldos[i].desc.min_uV;
						mtk_ldos[i].init_data.constraints.min_uV =
						    mtk_ldos[i].desc.min_uV;
						PMICLOG("test man_uv:%d min_uv:%d\n",
							(mtk_ldos[i].desc.n_voltages -
							 1) * mtk_ldos[i].desc.uV_step +
							mtk_ldos[i].desc.min_uV,
							mtk_ldos[i].desc.min_uV);
					}
				}
				PMICLOG("min_uv:%d max_uv:%d\n",
					mtk_ldos[i].init_data.constraints.min_uV,
					mtk_ldos[i].init_data.constraints.max_uV);
			}

			mtk_ldos[i].desc.owner = THIS_MODULE;

			mtk_ldos[i].rdev =
			    regulator_register(&mtk_ldos[i].desc, &mtk_ldos[i].config);
			if (IS_ERR(mtk_ldos[i].rdev)) {
				ret = PTR_ERR(mtk_ldos[i].rdev);
				PMICLOG("[regulator_register] failed to register %s (%d)\n",
					mtk_ldos[i].desc.name, ret);
			} else {
				PMICLOG("[regulator_register] pass to register %s\n",
					mtk_ldos[i].desc.name);
			}
			mtk_ldos[i].reg = regulator_get(&(dev->dev), mtk_ldos[i].desc.name);
			isEn = regulator_is_enabled(mtk_ldos[i].reg);
			if (isEn != 0) {
				PMICLOG("[regulator] %s is default on\n", mtk_ldos[i].desc.name);
				/*ret=regulator_enable(mtk_ldos[i].reg);*/
			}
		}
	}
#endif				/* End of #if !defined CONFIG_MTK_LEGACY */
	ret = register_pm_notifier(&pmic_regulator_pm_notifier_block);
	if (ret)
		PMICLOG("****failed to register PM notifier %d\n", ret);

}



void PMIC6351_regulator_test(void)
{
	int i = 0, j;
	int ret1, ret2;
	struct regulator *reg;

	for (i = 0; i < ARRAY_SIZE(mtk_ldos); i++) {
		if (mtk_ldos[i].isUsedable == 1) {
			reg = mtk_ldos[i].reg;
			PMICLOG("[regulator enable test] %s\n", mtk_ldos[i].desc.name);

			ret1 = regulator_enable(reg);
			ret2 = regulator_is_enabled(reg);

			if (ret2 == pmic_get_register_value(mtk_ldos[i].en_reg)) {
				PMICLOG("[enable test pass]\n");
			} else {
				PMICLOG("[enable test fail] ret = %d enable = %d addr:0x%x reg:0x%x\n",
				ret1, ret2, pmu_flags_table[mtk_ldos[i].en_reg].offset,
				pmic_get_register_value(mtk_ldos[i].en_reg));
			}

			ret1 = regulator_disable(reg);
			ret2 = regulator_is_enabled(reg);

			if (ret2 == pmic_get_register_value(mtk_ldos[i].en_reg)) {
				PMICLOG("[disable test pass]\n");
			} else {
				PMICLOG("[disable test fail] ret = %d enable = %d addr:0x%x reg:0x%x\n",
					ret1, ret2, pmu_flags_table[mtk_ldos[i].en_reg].offset,
					pmic_get_register_value(mtk_ldos[i].en_reg));
			}

		}
	}

	for (i = 0; i < ARRAY_SIZE(mtk_ldos); i++) {
		const int *pVoltage;

		reg = mtk_ldos[i].reg;
		if (mtk_ldos[i].isUsedable == 1) {
			PMICLOG("[regulator voltage test] %s voltage:%d\n",
				mtk_ldos[i].desc.name, mtk_ldos[i].desc.n_voltages);

			if (mtk_ldos[i].pvoltages != NULL) {
				pVoltage = (const int *)mtk_ldos[i].pvoltages;

				for (j = 0; j < mtk_ldos[i].desc.n_voltages; j++) {
					int rvoltage;

					regulator_set_voltage(reg, pVoltage[j], pVoltage[j]);
					rvoltage = regulator_get_voltage(reg);


					if ((j == pmic_get_register_value(mtk_ldos[i].vol_reg))
					    && (pVoltage[j] == rvoltage)) {
						PMICLOG
						    ("[%d:%d]:pass  set_voltage:%d  rvoltage:%d\n",
						     j,
						     pmic_get_register_value(mtk_ldos
									     [i].vol_reg),
						     pVoltage[j], rvoltage);

					} else {
						PMICLOG
						    ("[%d:%d]:fail  set_voltage:%d  rvoltage:%d\n",
						     j,
						     pmic_get_register_value(mtk_ldos
									     [i].vol_reg),
						     pVoltage[j], rvoltage);
					}
				}
			}
		}
	}


}

/*#if defined CONFIG_MTK_LEGACY*/
int getHwVoltage(MT65XX_POWER powerId)
{
#if defined CONFIG_MTK_LEGACY
	struct regulator *reg;


	if (powerId >= ARRAY_SIZE(mtk_ldos))
		return -100;

	if (mtk_ldos[powerId].isUsedable != true)
		return -101;

	reg = mtk_ldos[powerId].reg;

	return regulator_get_voltage(reg);
#else
	return -1;
#endif
}
EXPORT_SYMBOL(getHwVoltage);

int isHwPowerOn(MT65XX_POWER powerId)
{
#if defined CONFIG_MTK_LEGACY
	struct regulator *reg;

	if (powerId >= ARRAY_SIZE(mtk_ldos))
		return -100;

	if (mtk_ldos[powerId].isUsedable != true)
		return -101;

	reg = mtk_ldos[powerId].reg;

	return regulator_is_enabled(reg);
#else
	return -1;
#endif

}
EXPORT_SYMBOL(isHwPowerOn);

bool hwPowerOn(MT65XX_POWER powerId, int powerVolt, char *mode_name)
{
#if defined CONFIG_MTK_LEGACY
	struct regulator *reg;
	int ret1, ret2;

	if (powerId >= ARRAY_SIZE(mtk_ldos))
		return false;

	if (mtk_ldos[powerId].isUsedable != true)
		return false;

	reg = mtk_ldos[powerId].reg;

	ret2 = regulator_set_voltage(reg, powerVolt, powerVolt);

	if (ret2 != 0) {
		PMICLOG("hwPowerOn:%s can't set the same volt %d again.", mtk_ldos[powerId].desc.name, powerVolt);
		PMICLOG("Or please check:%s volt %d is correct.", mtk_ldos[powerId].desc.name, powerVolt);
	}

	ret1 = regulator_enable(reg);

	if (ret1 != 0)
		PMICLOG("hwPowerOn:%s enable fail", mtk_ldos[powerId].desc.name);

	PMICLOG("hwPowerOn:%d:%s volt:%d name:%s cnt:%d", powerId, mtk_ldos[powerId].desc.name,
		powerVolt, mode_name, mtk_ldos[powerId].rdev->use_count);
	return true;
#else
	return false;
#endif
}
EXPORT_SYMBOL(hwPowerOn);

bool hwPowerDown(MT65XX_POWER powerId, char *mode_name)
{
#if defined CONFIG_MTK_LEGACY
	struct regulator *reg;
	int ret1;

	if (powerId >= ARRAY_SIZE(mtk_ldos))
		return false;

	if (mtk_ldos[powerId].isUsedable != true)
		return false;
	reg = mtk_ldos[powerId].reg;
	ret1 = regulator_disable(reg);

	if (ret1 != 0)
		PMICLOG("hwPowerOn err:ret1:%d ", ret1);

	PMICLOG("hwPowerDown:%d:%s name:%s cnt:%d", powerId, mtk_ldos[powerId].desc.name, mode_name,
		mtk_ldos[powerId].rdev->use_count);
	return true;
#else
	return false;
#endif
}
EXPORT_SYMBOL(hwPowerDown);

bool hwPowerSetVoltage(MT65XX_POWER powerId, int powerVolt, char *mode_name)
{
#if defined CONFIG_MTK_LEGACY
	struct regulator *reg;
	int ret1;

	if (powerId >= ARRAY_SIZE(mtk_ldos))
		return false;

	reg = mtk_ldos[powerId].reg;

	ret1 = regulator_set_voltage(reg, powerVolt, powerVolt);

	if (ret1 != 0) {
		PMICLOG("hwPowerSetVoltage:%s can't set the same voltage %d", mtk_ldos[powerId].desc.name,
			powerVolt);
	}


	PMICLOG("hwPowerSetVoltage:%d:%s name:%s cnt:%d", powerId, mtk_ldos[powerId].desc.name,
		mode_name, mtk_ldos[powerId].rdev->use_count);
	return true;
#else
	return false;
#endif
}
EXPORT_SYMBOL(hwPowerSetVoltage);

/*#endif*/ /* End of #if defined CONFIG_MTK_LEGACY */

#if PMIC_UT
/* UT test code TBD */
void low_bat_test(LOW_BATTERY_LEVEL level_val)
{
	pr_err("[low_bat_test] get %d\n", level_val);
}

void bat_oc_test(BATTERY_OC_LEVEL level_val)
{
	pr_err("[bat_oc_test] get %d\n", level_val);
}

void bat_per_test(BATTERY_PERCENT_LEVEL level_val)
{
	pr_err("[bat_per_test] get %d\n", level_val);
}
#endif

/*****************************************************************************
 * Low battery call back function
 ******************************************************************************/
#define LBCB_NUM 16

#ifndef DISABLE_LOW_BATTERY_PROTECT
#define LOW_BATTERY_PROTECT
#endif

int g_lowbat_int_bottom = 0;

#ifdef LOW_BATTERY_PROTECT
/* ex. 3400/5400*4096*/
#define BAT_HV_THD   (POWER_INT0_VOLT*4096/5400)	/*ex: 3400mV*/
#define BAT_LV_1_THD (POWER_INT1_VOLT*4096/5400)	/*ex: 3250mV*/
#define BAT_LV_2_THD (POWER_INT2_VOLT*4096/5400)	/*ex: 3000mV*/

int g_low_battery_level = 0;
int g_low_battery_stop = 0;
/*give one change to ignore DLPT power off. battery voltage may return to 3.25 or higher
because loading become light. */
int g_low_battery_if_power_off = 0;

struct low_battery_callback_table {
	void *lbcb;
};

struct low_battery_callback_table lbcb_tb[] = {
	{NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL},
	{NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}
};
#endif
void (*low_battery_callback)(LOW_BATTERY_LEVEL);

void register_low_battery_notify(void (*low_battery_callback) (LOW_BATTERY_LEVEL),
				 LOW_BATTERY_PRIO prio_val)
{
#ifdef LOW_BATTERY_PROTECT
	PMICLOG("[register_low_battery_notify] start\n");

	lbcb_tb[prio_val].lbcb = low_battery_callback;

	pr_err("[register_low_battery_notify] prio_val=%d\n", prio_val);
#endif /*end of #ifdef LOW_BATTERY_PROTECT */
}
EXPORT_SYMBOL(register_low_battery_notify);
#ifdef LOW_BATTERY_PROTECT
void exec_low_battery_callback(LOW_BATTERY_LEVEL low_battery_level)
{				/*0:no limit */
	int i = 0;

	if (g_low_battery_stop == 1) {
		pr_err("[exec_low_battery_callback] g_low_battery_stop=%d\n", g_low_battery_stop);
	} else {
		pr_debug("[exec_low_battery_callback] prio_val=%d,low_battery=%d\n", i, low_battery_level);
		for (i = 0; i < LBCB_NUM; i++) {
			if (lbcb_tb[i].lbcb != NULL) {
				low_battery_callback = lbcb_tb[i].lbcb;
				low_battery_callback(low_battery_level);
				/*
					pr_debug
					    ("[exec_low_battery_callback] prio_val=%d,low_battery=%d\n",
					     i, low_battery_level);
					     */
			}
		}
	}
}

void lbat_min_en_setting(int en_val)
{
	pmic_set_register_value(PMIC_AUXADC_LBAT_EN_MIN, en_val);
	pmic_set_register_value(PMIC_AUXADC_LBAT_IRQ_EN_MIN, en_val);
	pmic_set_register_value(PMIC_RG_INT_EN_BAT_L, en_val);
}

void lbat_max_en_setting(int en_val)
{
	pmic_set_register_value(PMIC_AUXADC_LBAT_EN_MAX, en_val);
	pmic_set_register_value(PMIC_AUXADC_LBAT_IRQ_EN_MAX, en_val);
	pmic_set_register_value(PMIC_RG_INT_EN_BAT_H, en_val);
}

void low_battery_protect_init(void)
{
	/*default setting */
	pmic_set_register_value(PMIC_AUXADC_LBAT_DEBT_MIN, 0);
	pmic_set_register_value(PMIC_AUXADC_LBAT_DEBT_MAX, 0);
	pmic_set_register_value(PMIC_AUXADC_LBAT_DET_PRD_15_0, 1);
	pmic_set_register_value(PMIC_AUXADC_LBAT_DET_PRD_19_16, 0);

	pmic_set_register_value(PMIC_AUXADC_LBAT_VOLT_MAX, BAT_HV_THD);
	pmic_set_register_value(PMIC_AUXADC_LBAT_VOLT_MIN, BAT_LV_1_THD);

	lbat_min_en_setting(1);
	lbat_max_en_setting(0);

	pr_err("Reg[0x%x]=0x%x, Reg[0x%x]=0x%x, Reg[0x%x]=0x%x\n",
		MT6351_AUXADC_LBAT3, upmu_get_reg_value(MT6351_AUXADC_LBAT3),
		MT6351_AUXADC_LBAT4, upmu_get_reg_value(MT6351_AUXADC_LBAT4),
		MT6351_INT_CON0, upmu_get_reg_value(MT6351_INT_CON0)
	    );

	pr_err("[low_battery_protect_init] %d mV, %d mV, %d mV\n",
		POWER_INT0_VOLT, POWER_INT1_VOLT, POWER_INT2_VOLT);
	PMICLOG("[low_battery_protect_init] Done\n");

}

#endif				/*#ifdef LOW_BATTERY_PROTECT*/

/*****************************************************************************
 * Battery OC call back function
 ******************************************************************************/
#define OCCB_NUM 16

#ifndef DISABLE_BATTERY_OC_PROTECT
#define BATTERY_OC_PROTECT
#endif

#ifdef BATTERY_OC_PROTECT
/* ex. Ireg = 65535 - (I * 950000uA / 2 / 158.122 / CAR_TUNE_VALUE * 100)*/
/* (950000/2/158.122)*100~=300400*/

#if defined(CONFIG_MTK_SMART_BATTERY)
#define BAT_OC_H_THD   \
(65535-((300400*POWER_BAT_OC_CURRENT_H/1000)/batt_meter_cust_data.car_tune_value))	/*ex: 4670mA*/
#define BAT_OC_L_THD   \
(65535-((300400*POWER_BAT_OC_CURRENT_L/1000)/batt_meter_cust_data.car_tune_value))	/*ex: 5500mA*/

#define BAT_OC_H_THD_RE   \
(65535-((300400*POWER_BAT_OC_CURRENT_H_RE/1000)/batt_meter_cust_data.car_tune_value))	/*ex: 3400mA*/
#define BAT_OC_L_THD_RE   \
(65535-((300400*POWER_BAT_OC_CURRENT_L_RE/1000)/batt_meter_cust_data.car_tune_value))	/*ex: 4000mA*/
#else
#define BAT_OC_H_THD   0xc047
#define BAT_OC_L_THD   0xb4f4
#define BAT_OC_H_THD_RE  0xc047
#define BAT_OC_L_THD_RE   0xb4f4
#endif /* end of #if defined(CONFIG_MTK_SMART_BATTERY) */
int g_battery_oc_level = 0;
int g_battery_oc_stop = 0;

struct battery_oc_callback_table {
	void *occb;
};

struct battery_oc_callback_table occb_tb[] = {
	{NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL},
	{NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}
};
#endif /*end of #ifdef BATTERY_OC_PROTECT*/
void (*battery_oc_callback)(BATTERY_OC_LEVEL);

void register_battery_oc_notify(void (*battery_oc_callback) (BATTERY_OC_LEVEL),
				BATTERY_OC_PRIO prio_val)
{
#ifdef BATTERY_OC_PROTECT
	PMICLOG("[register_battery_oc_notify] start\n");

	occb_tb[prio_val].occb = battery_oc_callback;

	pr_err("[register_battery_oc_notify] prio_val=%d\n", prio_val);
#endif
}

void exec_battery_oc_callback(BATTERY_OC_LEVEL battery_oc_level)
{				/*0:no limit */
#ifdef BATTERY_OC_PROTECT
	int i = 0;

	if (g_battery_oc_stop == 1) {
		pr_err("[exec_battery_oc_callback] g_battery_oc_stop=%d\n", g_battery_oc_stop);
	} else {
		for (i = 0; i < OCCB_NUM; i++) {
			if (occb_tb[i].occb != NULL) {
				battery_oc_callback = occb_tb[i].occb;
				battery_oc_callback(battery_oc_level);
				pr_err
				    ("[exec_battery_oc_callback] prio_val=%d,battery_oc_level=%d\n",
				     i, battery_oc_level);
			}
		}
	}
#endif
}
#ifdef BATTERY_OC_PROTECT
void bat_oc_h_en_setting(int en_val)
{
	pmic_set_register_value(PMIC_RG_INT_EN_FG_CUR_H, en_val);
	/* mt6325_upmu_set_rg_int_en_fg_cur_h(en_val); */
}

void bat_oc_l_en_setting(int en_val)
{
	pmic_set_register_value(PMIC_RG_INT_EN_FG_CUR_L, en_val);
	/*mt6325_upmu_set_rg_int_en_fg_cur_l(en_val); */
}

void battery_oc_protect_init(void)
{
	pmic_set_register_value(PMIC_FG_CUR_HTH, BAT_OC_H_THD);
	/*mt6325_upmu_set_fg_cur_hth(BAT_OC_H_THD); */
	pmic_set_register_value(PMIC_FG_CUR_LTH, BAT_OC_L_THD);
	/*mt6325_upmu_set_fg_cur_lth(BAT_OC_L_THD); */

	bat_oc_h_en_setting(0);
	bat_oc_l_en_setting(1);

	pr_err("Reg[0x%x]=0x%x, Reg[0x%x]=0x%x, Reg[0x%x]=0x%x\n",
		MT6351_PMIC_FG_CUR_HTH_ADDR, upmu_get_reg_value(MT6351_PMIC_FG_CUR_HTH_ADDR),
		MT6351_PMIC_FG_CUR_LTH_ADDR, upmu_get_reg_value(MT6351_PMIC_FG_CUR_LTH_ADDR),
		MT6351_PMIC_RG_INT_EN_FG_BAT_H_ADDR, upmu_get_reg_value(MT6351_PMIC_RG_INT_EN_FG_BAT_H_ADDR)
	    );

	pr_err("[battery_oc_protect_init] %d mA, %d mA\n",
		POWER_BAT_OC_CURRENT_H, POWER_BAT_OC_CURRENT_L);
	PMICLOG("[battery_oc_protect_init] Done\n");
}

void battery_oc_protect_reinit(void)
{
#ifdef BATTERY_OC_PROTECT
	pmic_set_register_value(PMIC_FG_CUR_HTH, BAT_OC_H_THD_RE);
	/*mt6325_upmu_set_fg_cur_hth(BAT_OC_H_THD_RE); */
	pmic_set_register_value(PMIC_FG_CUR_LTH, BAT_OC_L_THD_RE);
	/*mt6325_upmu_set_fg_cur_lth(BAT_OC_L_THD_RE); */

	pr_err("Reg[0x%x]=0x%x, Reg[0x%x]=0x%x, Reg[0x%x]=0x%x\n",
		MT6351_PMIC_FG_CUR_HTH_ADDR, upmu_get_reg_value(MT6351_PMIC_FG_CUR_HTH_ADDR),
		MT6351_PMIC_FG_CUR_LTH_ADDR, upmu_get_reg_value(MT6351_PMIC_FG_CUR_LTH_ADDR),
		MT6351_PMIC_RG_INT_EN_FG_BAT_H_ADDR, upmu_get_reg_value(MT6351_PMIC_RG_INT_EN_FG_BAT_H_ADDR)
	    );

	pr_err("[battery_oc_protect_reinit] %d mA, %d mA\n",
		POWER_BAT_OC_CURRENT_H_RE, POWER_BAT_OC_CURRENT_L_RE);
	pr_err("[battery_oc_protect_reinit] Done\n");
#else
	pr_warn("[battery_oc_protect_reinit] no define BATTERY_OC_PROTECT\n");
#endif
}
#endif /* #ifdef BATTERY_OC_PROTECT */


/*****************************************************************************
 * 15% notify service
 ******************************************************************************/
#ifndef DISABLE_BATTERY_PERCENT_PROTECT
#define BATTERY_PERCENT_PROTECT
#endif

#ifdef BATTERY_PERCENT_PROTECT
static struct hrtimer bat_percent_notify_timer;
static struct task_struct *bat_percent_notify_thread;
static bool bat_percent_notify_flag;
static DECLARE_WAIT_QUEUE_HEAD(bat_percent_notify_waiter);
#endif
#if !defined CONFIG_HAS_WAKELOCKS
struct wakeup_source bat_percent_notify_lock;
#else
struct wake_lock bat_percent_notify_lock;
#endif

static DEFINE_MUTEX(bat_percent_notify_mutex);

#ifdef BATTERY_PERCENT_PROTECT
/*extern unsigned int bat_get_ui_percentage(void);*/

#define BPCB_NUM 16

int g_battery_percent_level = 0;
int g_battery_percent_stop = 0;

#define BAT_PERCENT_LINIT 15

struct battery_percent_callback_table {
	void *bpcb;
};

struct battery_percent_callback_table bpcb_tb[] = {
	{NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL},
	{NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}
};
#endif /* end of #ifdef BATTERY_PERCENT_PROTECT */
void (*battery_percent_callback)(BATTERY_PERCENT_LEVEL);

void register_battery_percent_notify(void (*battery_percent_callback)(BATTERY_PERCENT_LEVEL),
				     BATTERY_PERCENT_PRIO prio_val)
{
#ifdef BATTERY_PERCENT_PROTECT
	PMICLOG("[register_battery_percent_notify] start\n");

	bpcb_tb[prio_val].bpcb = battery_percent_callback;

	pr_err("[register_battery_percent_notify] prio_val=%d\n", prio_val);

	if ((g_battery_percent_stop == 0) && (g_battery_percent_level == 1)) {
#ifdef DISABLE_DLPT_FEATURE
		pr_err("[register_battery_percent_notify] level l happen\n");
		battery_percent_callback(BATTERY_PERCENT_LEVEL_1);
#else
		if (prio_val == BATTERY_PERCENT_PRIO_FLASHLIGHT) {
			pr_err("[register_battery_percent_notify at DLPT] level l happen\n");
			battery_percent_callback(BATTERY_PERCENT_LEVEL_1);
		}
#endif
	}
#endif /* end of #ifdef BATTERY_PERCENT_PROTECT */
}
EXPORT_SYMBOL(register_battery_percent_notify);

#ifdef BATTERY_PERCENT_PROTECT
void exec_battery_percent_callback(BATTERY_PERCENT_LEVEL battery_percent_level)
{				/*0:no limit */
#ifdef DISABLE_DLPT_FEATURE
	int i = 0;
#endif

	if (g_battery_percent_stop == 1) {
		pr_err("[exec_battery_percent_callback] g_battery_percent_stop=%d\n",
			g_battery_percent_stop);
	} else {
#ifdef DISABLE_DLPT_FEATURE
		for (i = 0; i < BPCB_NUM; i++) {
			if (bpcb_tb[i].bpcb != NULL) {
				battery_percent_callback = bpcb_tb[i].bpcb;
				battery_percent_callback(battery_percent_level);
				pr_err
				    ("[exec_battery_percent_callback] prio_val=%d,battery_percent_level=%d\n",
				     i, battery_percent_level);
			}
		}
#else
		battery_percent_callback = bpcb_tb[BATTERY_PERCENT_PRIO_FLASHLIGHT].bpcb;
		battery_percent_callback(battery_percent_level);
		pr_err
		    ("[exec_battery_percent_callback at DLPT] prio_val=%d,battery_percent_level=%d\n",
		     BATTERY_PERCENT_PRIO_FLASHLIGHT, battery_percent_level);
#endif
	}
}

int bat_percent_notify_handler(void *unused)
{
	ktime_t ktime;
	int bat_per_val = 0;

	do {
		ktime = ktime_set(10, 0);

		wait_event_interruptible(bat_percent_notify_waiter,
					 (bat_percent_notify_flag == true));

#if !defined CONFIG_HAS_WAKELOCKS
		__pm_stay_awake(&bat_percent_notify_lock);
#else
		wake_lock(&bat_percent_notify_lock);
#endif
		mutex_lock(&bat_percent_notify_mutex);

#if defined(CONFIG_MTK_SMART_BATTERY)
		bat_per_val = bat_get_ui_percentage();
#endif
		if ((upmu_get_rgs_chrdet() == 0) && (g_battery_percent_level == 0)
		    && (bat_per_val <= BAT_PERCENT_LINIT)) {
			g_battery_percent_level = 1;
			exec_battery_percent_callback(BATTERY_PERCENT_LEVEL_1);
		} else if ((g_battery_percent_level == 1) && (bat_per_val > BAT_PERCENT_LINIT)) {
			g_battery_percent_level = 0;
			exec_battery_percent_callback(BATTERY_PERCENT_LEVEL_0);
		} else {
		}
		bat_percent_notify_flag = false;

		PMICLOG("bat_per_level=%d,bat_per_val=%d\n", g_battery_percent_level, bat_per_val);

		mutex_unlock(&bat_percent_notify_mutex);
#if !defined CONFIG_HAS_WAKELOCKS
		__pm_relax(&bat_percent_notify_lock);
#else
		wake_unlock(&bat_percent_notify_lock);
#endif

		hrtimer_start(&bat_percent_notify_timer, ktime, HRTIMER_MODE_REL);

	} while (!kthread_should_stop());

	return 0;
}

enum hrtimer_restart bat_percent_notify_task(struct hrtimer *timer)
{
	bat_percent_notify_flag = true;
	wake_up_interruptible(&bat_percent_notify_waiter);
	PMICLOG("bat_percent_notify_task is called\n");

	return HRTIMER_NORESTART;
}

void bat_percent_notify_init(void)
{
	ktime_t ktime;

	ktime = ktime_set(20, 0);
	hrtimer_init(&bat_percent_notify_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	bat_percent_notify_timer.function = bat_percent_notify_task;
	hrtimer_start(&bat_percent_notify_timer, ktime, HRTIMER_MODE_REL);

	bat_percent_notify_thread =
	    kthread_run(bat_percent_notify_handler, 0, "bat_percent_notify_thread");
	if (IS_ERR(bat_percent_notify_thread))
		pr_err("Failed to create bat_percent_notify_thread\n");
	else
		pr_err("Create bat_percent_notify_thread : done\n");
}
#endif /* #ifdef BATTERY_PERCENT_PROTECT */

/*****************************************************************************
 * DLPT service
 ******************************************************************************/
#ifndef DISABLE_DLPT_FEATURE
#define DLPT_FEATURE_SUPPORT
#endif

#ifdef DLPT_FEATURE_SUPPORT

unsigned int ptim_bat_vol = 0;
signed int ptim_R_curr = 0;
int ptim_imix = 0;
int ptim_rac_val_avg = 0;

signed int pmic_ptimretest = 0;



unsigned int ptim_cnt = 0;

signed int count_time_out_adc_imp = 36;
unsigned int count_adc_imp = 0;


int do_ptim_internal(bool isSuspend, unsigned int *bat, signed int *cur)
{
	unsigned int vbat_reg;
	int ret = 0;

	count_adc_imp = 0;
	/*PMICLOG("[do_ptim] start\n"); */

	/*pmic_set_register_value(PMIC_RG_AUXADC_RST,1); */
	/*pmic_set_register_value(PMIC_RG_AUXADC_RST,0); */

	upmu_set_reg_value(0x0eac, 0x0006);

	pmic_set_register_value(PMIC_AUXADC_IMP_AUTORPT_PRD, 6);
	pmic_set_register_value(PMIC_RG_AUXADC_SMPS_CK_PDN, 0);
	pmic_set_register_value(PMIC_RG_AUXADC_SMPS_CK_PDN_HWEN, 0);

	pmic_set_register_value(PMIC_RG_AUXADC_CK_PDN_HWEN, 0);
	pmic_set_register_value(PMIC_RG_AUXADC_CK_PDN, 0);

	pmic_set_register_value(PMIC_AUXADC_CLR_IMP_CNT_STOP, 1);
	pmic_set_register_value(PMIC_AUXADC_IMPEDANCE_IRQ_CLR, 1);

	/*restore to initial state */
	pmic_set_register_value(PMIC_AUXADC_CLR_IMP_CNT_STOP, 0);
	pmic_set_register_value(PMIC_AUXADC_IMPEDANCE_IRQ_CLR, 0);

	/*set issue interrupt */
	/*pmic_set_register_value(PMIC_RG_INT_EN_AUXADC_IMP,1); */

#if defined(SWCHR_POWER_PATH)
	pmic_set_register_value(PMIC_AUXADC_IMPEDANCE_CHSEL, 1);
#else
	pmic_set_register_value(PMIC_AUXADC_IMPEDANCE_CHSEL, 0);
#endif
	pmic_set_register_value(PMIC_AUXADC_IMP_AUTORPT_EN, 1);
	pmic_set_register_value(PMIC_AUXADC_IMPEDANCE_CNT, 3);
	pmic_set_register_value(PMIC_AUXADC_IMPEDANCE_MODE, 1);

/*      MT6351_PMICLOG("[do_ptim] end %d %d\n",pmic_get_register_value
(MT6351_PMIC_RG_AUXADC_SMPS_CK_PDN),pmic_get_register_value(
MT6351_PMIC_RG_AUXADC_SMPS_CK_PDN_HWEN));*/

	while (pmic_get_register_value(PMIC_AUXADC_IMPEDANCE_IRQ_STATUS) == 0) {
		/*MT6351_PMICLOG("[do_ptim] MT6351_PMIC_AUXADC_IMPEDANCE_IRQ_STATUS= %d\n",
		   pmic_get_register_value(PMIC_AUXADC_IMPEDANCE_IRQ_STATUS)); */
		if ((count_adc_imp++) > count_time_out_adc_imp) {
			pr_err("do_ptim over %d times/ms\n", count_adc_imp);
			ret = 1;
			break;
		}
		mdelay(1);
	}

	/*disable */
	pmic_set_register_value(PMIC_AUXADC_IMP_AUTORPT_EN, 0);	/*typo */
	pmic_set_register_value(PMIC_AUXADC_IMPEDANCE_MODE, 0);

	/*clear irq */
	pmic_set_register_value(PMIC_AUXADC_CLR_IMP_CNT_STOP, 1);
	pmic_set_register_value(PMIC_AUXADC_IMPEDANCE_IRQ_CLR, 1);
	pmic_set_register_value(PMIC_AUXADC_CLR_IMP_CNT_STOP, 0);
	pmic_set_register_value(PMIC_AUXADC_IMPEDANCE_IRQ_CLR, 0);

	/*PMICLOG("[do_ptim2] 0xee8=0x%x  0x2c6=0x%x\n", upmu_get_reg_value
	(0xee8),upmu_get_reg_value(0x2c6));*/


	/*pmic_set_register_value(PMIC_RG_INT_STATUS_AUXADC_IMP,1);write 1 to clear ! */
	/*pmic_set_register_value(PMIC_RG_INT_EN_AUXADC_IMP,0); */


	vbat_reg = pmic_get_register_value(PMIC_AUXADC_ADC_OUT_IMP_AVG);
	/*ptim_bat_vol = (vbat_reg * 3 * 18000) / 32768;*/
	*bat = (vbat_reg * 3 * 18000) / 32768;

#if defined(CONFIG_MTK_SMART_BATTERY)
	fgauge_read_IM_current((void *)cur);
#else
	*cur = 0;
#endif
/*	pr_err("do_ptim_internal : bat %d cur %d\n", *bat, *cur); */

#if defined(SWCHR_POWER_PATH)
/*	pr_err("do_ptim_internal test: bat %d cur %d\n", *bat, *cur); */
#endif



	return ret;
}

int do_ptim(bool isSuspend)
{
	int ret;

	if (isSuspend == false)
		pmic_auxadc_lock();

	ret = do_ptim_internal(isSuspend, &ptim_bat_vol, &ptim_R_curr);

	if (isSuspend == false)
		pmic_auxadc_unlock();
	return ret;
}

int do_ptim_ex(bool isSuspend, unsigned int *bat, signed int *cur)
{
	int ret;

	if (isSuspend == false)
		pmic_auxadc_lock();

	ret = do_ptim_internal(isSuspend, bat, cur);

	if (isSuspend == false)
		pmic_auxadc_unlock();
	return ret;
}

void get_ptim_value(bool isSuspend, unsigned int *bat, signed int *cur)
{
	if (isSuspend == false)
		pmic_auxadc_lock();
	*bat = ptim_bat_vol;
	*cur = ptim_R_curr;
	if (isSuspend == false)
		pmic_auxadc_unlock();
}



void enable_dummy_load(unsigned int en)
{

	if (en == 1) {
		/*1. disable isink pdn */
		pmic_set_register_value(PMIC_RG_DRV_ISINK3_CK_PDN, 0);
		pmic_set_register_value(PMIC_RG_DRV_ISINK2_CK_PDN, 0);
		/*
		pmic_set_register_value(PMIC_RG_DRV_ISINK1_CK_PDN, 0);
		pmic_set_register_value(PMIC_RG_DRV_ISINK0_CK_PDN, 0);
		*/
		pmic_set_register_value(PMIC_RG_DRV_32K_CK_PDN, 0);

		/*1. disable isink pdn */
		pmic_set_register_value(PMIC_RG_DRV_CHRIND_CK_PDN, 0);
		pmic_set_register_value(PMIC_RG_DRV_ISINK7_CK_PDN, 0);
		pmic_set_register_value(PMIC_RG_DRV_ISINK6_CK_PDN, 0);
		/*
		pmic_set_register_value(PMIC_RG_DRV_ISINK5_CK_PDN, 0);
		pmic_set_register_value(PMIC_RG_DRV_ISINK4_CK_PDN, 0);
		*/

		/* enable isink step */
		pmic_set_register_value(PMIC_ISINK_CH2_STEP, 0x7);
		pmic_set_register_value(PMIC_ISINK_CH3_STEP, 0x7);
		pmic_set_register_value(PMIC_ISINK_CH6_STEP, 0x7);
		pmic_set_register_value(PMIC_ISINK_CH7_STEP, 0x7);

		/*enable isink */
		pmic_set_register_value(PMIC_ISINK_CH7_BIAS_EN, 0x1);
		pmic_set_register_value(PMIC_ISINK_CH6_BIAS_EN, 0x1);
		pmic_set_register_value(PMIC_ISINK_CH3_BIAS_EN, 0x1);
		pmic_set_register_value(PMIC_ISINK_CH2_BIAS_EN, 0x1);
		pmic_set_register_value(PMIC_ISINK_CHOP7_EN, 0x1);
		pmic_set_register_value(PMIC_ISINK_CHOP6_EN, 0x1);
		pmic_set_register_value(PMIC_ISINK_CHOP3_EN, 0x1);
		pmic_set_register_value(PMIC_ISINK_CHOP2_EN, 0x1);
		pmic_set_register_value(PMIC_ISINK_CH7_EN, 0x1);
		pmic_set_register_value(PMIC_ISINK_CH6_EN, 0x1);
		pmic_set_register_value(PMIC_ISINK_CH3_EN, 0x1);
		pmic_set_register_value(PMIC_ISINK_CH2_EN, 0x1);
		/*PMICLOG("[enable dummy load]\n"); */
	} else {
		/*upmu_set_reg_value(0x828,0x0cc0); */
#if defined MT6328
		pmic_set_register_value(PMIC_ISINK_CH2_EN, 0);
		pmic_set_register_value(PMIC_ISINK_CH3_EN, 0);
#endif
		pmic_set_register_value(PMIC_ISINK_CH7_EN, 0);
		pmic_set_register_value(PMIC_ISINK_CH6_EN, 0);
		pmic_set_register_value(PMIC_ISINK_CH3_EN, 0);
		pmic_set_register_value(PMIC_ISINK_CH2_EN, 0);
		pmic_set_register_value(PMIC_ISINK_CHOP7_EN, 0);
		pmic_set_register_value(PMIC_ISINK_CHOP6_EN, 0);
		pmic_set_register_value(PMIC_ISINK_CHOP3_EN, 0);
		pmic_set_register_value(PMIC_ISINK_CHOP2_EN, 0);
		pmic_set_register_value(PMIC_ISINK_CH7_BIAS_EN, 0);
		pmic_set_register_value(PMIC_ISINK_CH6_BIAS_EN, 0);
		pmic_set_register_value(PMIC_ISINK_CH3_BIAS_EN, 0);
		pmic_set_register_value(PMIC_ISINK_CH2_BIAS_EN, 0);

		/*1. enable isink pdn */
		pmic_set_register_value(PMIC_RG_DRV_ISINK3_CK_PDN, 0x1);
		pmic_set_register_value(PMIC_RG_DRV_ISINK2_CK_PDN, 0x1);
		pmic_set_register_value(PMIC_RG_DRV_32K_CK_PDN, 0x1);

		/*1. enable isink pdn */
		pmic_set_register_value(PMIC_RG_DRV_CHRIND_CK_PDN, 0x1);
		pmic_set_register_value(PMIC_RG_DRV_ISINK7_CK_PDN, 0x1);
		pmic_set_register_value(PMIC_RG_DRV_ISINK6_CK_PDN, 0x1);

		/*pmic_set_register_value(PMIC_RG_VIBR_EN,0); */
		/*PMICLOG("[disable dummy load]\n"); */
	}

}

#endif /* #ifdef DLPT_FEATURE_SUPPORT */

#ifdef DLPT_FEATURE_SUPPORT
static struct hrtimer dlpt_notify_timer;
static struct task_struct *dlpt_notify_thread;
static bool dlpt_notify_flag;
static DECLARE_WAIT_QUEUE_HEAD(dlpt_notify_waiter);
#endif
#if !defined CONFIG_HAS_WAKELOCKS
struct wakeup_source dlpt_notify_lock;
#else
struct wake_lock dlpt_notify_lock;
#endif
static DEFINE_MUTEX(dlpt_notify_mutex);

#ifdef DLPT_FEATURE_SUPPORT
#define DLPT_NUM 16
/* This define is used to filter smallest and largest voltage and current.
To avoid auxadc measurement interference issue. */
#define DLPT_SORT_IMIX_VOLT_CURR 1


int g_dlpt_stop = 0;
unsigned int g_dlpt_val = 0;

int g_dlpt_start = 0;


int g_imix_val = 0;
int g_imix_val_pre = 0;
int g_low_per_timer = 0;
int g_low_per_timeout_val = 60;


int g_lbatInt1 = POWER_INT2_VOLT * 10;

struct dlpt_callback_table {
	void *dlpt_cb;
};

struct dlpt_callback_table dlpt_cb_tb[] = {
	{NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL},
	{NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}
};

void (*dlpt_callback)(unsigned int);

void register_dlpt_notify(void (*dlpt_callback) (unsigned int), DLPT_PRIO prio_val)
{
	PMICLOG("[register_dlpt_notify] start\n");

	dlpt_cb_tb[prio_val].dlpt_cb = dlpt_callback;

	pr_err("[register_dlpt_notify] prio_val=%d\n", prio_val);

	if ((g_dlpt_stop == 0) && (g_dlpt_val != 0)) {
		pr_err("[register_dlpt_notify] dlpt happen\n");
		dlpt_callback(g_dlpt_val);
	}
}

void exec_dlpt_callback(unsigned int dlpt_val)
{
	int i = 0;

	g_dlpt_val = dlpt_val;

	if (g_dlpt_stop == 1) {
		pr_err("[exec_dlpt_callback] g_dlpt_stop=%d\n", g_dlpt_stop);
	} else {
		for (i = 0; i < DLPT_NUM; i++) {
			if (dlpt_cb_tb[i].dlpt_cb != NULL) {
				dlpt_callback = dlpt_cb_tb[i].dlpt_cb;
				dlpt_callback(g_dlpt_val);
				/* pr_debug("[exec_dlpt_callback] g_dlpt_val=%d\n", g_dlpt_val); */
			}
		}
	}
}

/*
int get_dlpt_iavg(int is_use_zcv)
{
	int bat_cap_val = 0;
	int zcv_val = 0;
	int vsys_min_2_val = POWER_INT2_VOLT;
	int rbat_val = 0;
	int rdc_val = 0;
	int iavg_val = 0;

	bat_cap_val = bat_get_ui_percentage();

	if(is_use_zcv == 1)
		zcv_val = fgauge_read_v_by_d(100-bat_cap_val);
	else
	{
	#if defined(SWCHR_POWER_PATH)
		zcv_val = PMIC_IMM_GetOneChannelValue(MT6351_PMIC_AUX_ISENSE_AP,5,1);
	#else
		zcv_val = PMIC_IMM_GetOneChannelValue(MT6351_PMIC_AUX_BATSNS_AP,5,1);
	#endif
	}
	rbat_val = fgauge_read_r_bat_by_v(zcv_val);
	rdc_val = CUST_R_FG_OFFSET+R_FG_VALUE+rbat_val;

	if(rdc_val==0)
		rdc_val=1;
	iavg_val = ((zcv_val-vsys_min_2_val)*1000)/rdc_val;

	return iavg_val;
}

int get_real_volt(int val)*/ /*0.1mV*/
/*
{
    int ret = 0;
    ret = val&0x7FFF;
    ret = (ret*4*1800*10)/32768;
    return ret;
}

int get_real_curr(int val)*/ /*0.1mA*/
/*
{

	int ret=0;

	if ( val > 32767 ) {
		ret = val-65535;
		ret = ret-(ret*2);
	} else
		ret = val;
	ret = ret*158122;
	do_div(ret, 100000);
	ret = (ret*20)/R_FG_VALUE;
	ret = ((ret*CAR_TUNE_VALUE)/100);

	return ret;
}
*/
int get_rac_val(void)
{
	int volt_1 = 0;
	int volt_2 = 0;
	int curr_1 = 0;
	int curr_2 = 0;
	int rac_cal = 0;
	int ret = 0;
	bool retry_state = false;
	int retry_count = 0;

	do {
		/*adc and fg-------------------------------------------------------- */
		do_ptim(true);

		pmic_spm_crit2("[1,Trigger ADC PTIM mode] volt1=%d, curr_1=%d\n", ptim_bat_vol,
			       ptim_R_curr);
		volt_1 = ptim_bat_vol;
		curr_1 = ptim_R_curr;

		pmic_spm_crit2("[2,enable dummy load]");
		enable_dummy_load(1);
		mdelay(50);
		/*Wait --------------------------------------------------------------*/

		/*adc and fg-------------------------------------------------------- */
		do_ptim(true);

		pmic_spm_crit2("[3,Trigger ADC PTIM mode again] volt2=%d, curr_2=%d\n",
			       ptim_bat_vol, ptim_R_curr);
		volt_2 = ptim_bat_vol;
		curr_2 = ptim_R_curr;

		/*Disable dummy load-------------------------------------------------*/
		enable_dummy_load(0);

		/*Calculate Rac------------------------------------------------------ */
		if ((curr_2 - curr_1) >= 700 && (curr_2 - curr_1) <= 1200
		    && (volt_1 - volt_2) >= 80) {
			/*40.0mA */
			rac_cal = ((volt_1 - volt_2) * 1000) / (curr_2 - curr_1);	/*m-ohm */

			if (rac_cal < 0)
				ret = (rac_cal - (rac_cal * 2)) * 1;
			else
				ret = rac_cal * 1;

		} else {
			ret = -1;
			pmic_spm_crit2("[4,Calculate Rac] bypass due to (curr_x-curr_y) < 40mA\n");
		}

		pmic_spm_crit2
		    ("volt_1 = %d,volt_2 = %d,curr_1 = %d,curr_2 = %d,rac_cal = %d,ret = %d,retry_count = %d\n",
		     volt_1, volt_2, curr_1, curr_2, rac_cal, ret, retry_count);

		pmic_spm_crit2(" %d,%d,%d,%d,%d,%d,%d\n",
			       volt_1, volt_2, curr_1, curr_2, rac_cal, ret, retry_count);


		/*------------------------*/
		retry_count++;

		if ((retry_count < 3) && (ret == -1))
			retry_state = true;
		else
			retry_state = false;

	} while (retry_state == true);

	return ret;
}

int get_dlpt_imix_spm(void)
{
#if defined(CONFIG_MTK_SMART_BATTERY)
	int rac_val[5], rac_val_avg;
#if 0
	int volt[5], curr[5];
	int volt_avg = 0, curr_avg = 0;
	int imix;
#endif
	int i;
	static unsigned int pre_ui_soc = 101;
	unsigned int ui_soc;

	ui_soc = bat_get_ui_percentage();

	if (ui_soc != pre_ui_soc) {
		pre_ui_soc = ui_soc;
	} else {
		pmic_spm_crit2("[dlpt_R] pre_SOC=%d SOC=%d skip\n", pre_ui_soc, ui_soc);
		return 0;
	}


	for (i = 0; i < 2; i++) {
		rac_val[i] = get_rac_val();
		if (rac_val[i] == -1)
			return -1;
	}

	/*rac_val_avg=rac_val[0]+rac_val[1]+rac_val[2]+rac_val[3]+rac_val[4];*/
	/*rac_val_avg=rac_val_avg/5;*/
	/*PMICLOG("[dlpt_R] %d,%d,%d,%d,%d %d\n",rac_val[0],rac_val[1],rac_val[2],rac_val[3],rac_val[4],rac_val_avg);*/

	rac_val_avg = rac_val[0] + rac_val[1];
	rac_val_avg = rac_val_avg / 2;
	/*pmic_spm_crit2("[dlpt_R] %d,%d,%d\n", rac_val[0], rac_val[1], rac_val_avg);*/
	pr_err("[dlpt_R] %d,%d,%d\n", rac_val[0], rac_val[1], rac_val_avg);

	if (rac_val_avg > 100)
		ptim_rac_val_avg = rac_val_avg;

/*
	for(i=0;i<5;i++)
	{
		do_ptim();

		volt[i]=ptim_bat_vol;
		curr[i]=ptim_R_curr;
		volt_avg+=ptim_bat_vol;
		curr_avg+=ptim_R_curr;
	}

	volt_avg=volt_avg/5;
	curr_avg=curr_avg/5;

	imix=curr_avg+(volt_avg-g_lbatInt1)*1000/ptim_rac_val_avg;

		pmic_spm_crit2("[dlpt_Imix] %d,%d,%d,%d,%d,%d,%d\n",volt_avg,curr_avg,g_lbatInt1,
		ptim_rac_val_avg,imix,BMT_status.SOC,bat_get_ui_percentage());

	ptim_imix=imix;
*/
#endif /* end of #if defined(CONFIG_MTK_SMART_BATTERY) */
	return 0;

}

#if DLPT_SORT_IMIX_VOLT_CURR
void pmic_swap(int *a, int *b)
{
	int temp = *a;
	*a = *b;
	*b = temp;
}


void pmic_quicksort(int *data, int left, int right)
{
	int pivot, i, j;

	if (left >= right)
		return;

	pivot = data[left];

	i = left + 1;
	j = right;

	while (1) {
		while (i <= right) {
			if (data[i] > pivot)
				break;
			i = i + 1;
		}

		while (j > left) {
			if (data[j] < pivot)
				break;
			j = j - 1;
		}

		if (i > j)
			break;

		pmic_swap(&data[i], &data[j]);
	}

	pmic_swap(&data[left], &data[j]);

	pmic_quicksort(data, left, j - 1);
	pmic_quicksort(data, j + 1, right);
}
#endif /* end of DLPT_SORT_IMIX_VOLT_CURR*/
int get_dlpt_imix(void)
{
	/* int rac_val[5], rac_val_avg; */
	int volt[5], curr[5], volt_avg = 0, curr_avg = 0;
	int imix;
#if 0 /* debug only */
	int val, val1, val2, val3, val4, val5, ret_val;
#endif
	int i, count_do_ptim = 0;

	for (i = 0; i < 5; i++) {
		/*adc and fg-------------------------------------------------------- */
		/* do_ptim(false); */
		while (do_ptim(false)) {
			if ((count_do_ptim >= 2) && (count_do_ptim < 4))
				pr_err("do_ptim more than twice times\n");
			else if (count_do_ptim > 3) {
				pr_err("do_ptim more than five times\n");
				BUG_ON(1);
			} else
				;
			count_do_ptim++;
		}

		volt[i] = ptim_bat_vol;
		curr[i] = ptim_R_curr;
#if !DLPT_SORT_IMIX_VOLT_CURR
		volt_avg += ptim_bat_vol;
		curr_avg += ptim_R_curr;
#endif
#if 0 /* debug only */
	PMICLOG("[get_dlpt_imix:%d] %d,%d,%d,%d\n", i, volt[i], curr[i], volt_avg, curr_avg);
	ret_val = pmic_read_interface((unsigned int)(MT6351_AUXADC_ADC29), (&val), (0xffff), 0);
	ret_val = pmic_read_interface((unsigned int)(MT6351_AUXADC_ADC30), (&val1), (0xffff), 0);
	ret_val = pmic_read_interface((unsigned int)(MT6351_FGADC_CON25), (&val2), (0xffff), 0);
	ret_val = pmic_read_interface((unsigned int)(MT6351_AUXADC_IMP0), (&val3), (0xffff), 0);
	ret_val = pmic_read_interface((unsigned int)(MT6351_AUXADC_CON2), (&val4), (0xffff), 0);
	ret_val = pmic_read_interface((unsigned int)(MT6351_AUXADC_LBAT1), (&val5), (0xffff), 0);
	pr_debug("[get_dlpt_imix after ptim]MT6351_AUXADC_ADC29 0x%x:0x%x, MT6351_AUXADC_ADC30 0x%x:0x%x, MT6351_FGADC_CON25 0x%x:0x%x\n",
		MT6351_AUXADC_ADC29, val, MT6351_AUXADC_ADC30, val1, MT6351_FGADC_CON25, val2);
	pr_debug("[get_dlpt_imix after ptim]MT6351_AUXADC_IMP0 0x%x:0x%x, MT6351_AUXADC_CON2 0x%x:0x%x, MT6351_AUXADC_LBAT1 0x%x:0x%x\n",
		MT6351_AUXADC_IMP0, val3, MT6351_AUXADC_CON2, val4, MT6351_AUXADC_LBAT1, val5);
	ret_val = pmic_read_interface((unsigned int)(MT6351_AUXADC_LBAT2), (&val), (0xffff), 0);
	pr_debug("[get_dlpt_imix after ptim]MT6351_AUXADC_LBAT2 0x%x:0x%x\n",
		MT6351_AUXADC_LBAT2, val);
#endif
	}
#if !DLPT_SORT_IMIX_VOLT_CURR
	volt_avg = volt_avg / 5;
	curr_avg = curr_avg / 5;
#else
	pmic_quicksort(volt, 0, 4);
	pmic_quicksort(curr, 0, 4);
	volt_avg = volt[1] + volt[2] + volt[3];
	curr_avg = curr[1] + curr[2] + curr[3];
	volt_avg = volt_avg / 3;
	curr_avg = curr_avg / 3;
#endif /* end of DLPT_SORT_IMIX_VOLT_CURR*/
	imix = (curr_avg + (volt_avg - g_lbatInt1) * 1000 / ptim_rac_val_avg) / 10;

#if defined(CONFIG_MTK_SMART_BATTERY)
	pr_debug("[get_dlpt_imix] %d,%d,%d,%d,%d,%d,%d\n", volt_avg, curr_avg, g_lbatInt1,
		ptim_rac_val_avg, imix, BMT_status.SOC, bat_get_ui_percentage());
#endif

	ptim_imix = imix;

	return ptim_imix;

}



int get_dlpt_imix_charging(void)
{

	int zcv_val = 0;
	int vsys_min_1_val = POWER_INT2_VOLT;
	int imix_val = 0;
#if defined(SWCHR_POWER_PATH)
	zcv_val = PMIC_IMM_GetOneChannelValue(PMIC_AUX_ISENSE_AP, 5, 1);
#else
	zcv_val = PMIC_IMM_GetOneChannelValue(PMIC_AUX_BATSNS_AP, 5, 1);
#endif

	imix_val = (zcv_val - vsys_min_1_val) * 1000 / ptim_rac_val_avg * 9 / 10;
	PMICLOG("[dlpt] get_dlpt_imix_charging %d %d %d %d\n",
		imix_val, zcv_val, vsys_min_1_val, ptim_rac_val_avg);

	return imix_val;
}


int dlpt_check_power_off(void)
{
	int ret = 0;

	ret = 0;
	if (g_dlpt_start == 0) {
		PMICLOG("[dlpt_check_power_off] not start\n");
	} else {
#ifdef LOW_BATTERY_PROTECT
		if (g_low_battery_level == 2 && g_lowbat_int_bottom == 1) {
			/*1st time receive battery voltage < 3.1V, record it */
			if (g_low_battery_if_power_off == 0) {
				g_low_battery_if_power_off++;
				pr_err("[dlpt_check_power_off] %d\n", g_low_battery_if_power_off);
			} else {
				/*2nd time receive battery voltage < 3.1V, wait FG to call power off */
				ret = 1;
				pr_err("[dlpt_check_power_off] %d %d\n", ret, g_low_battery_if_power_off);
			}
		} else {
			ret = 0;
			/* battery voltage > 3.1V, ignore it */
			g_low_battery_if_power_off = 0;
		}
#endif

		PMICLOG("[dlpt_check_power_off]");
		PMICLOG("ptim_imix=%d, POWEROFF_BAT_CURRENT=%d", ptim_imix, POWEROFF_BAT_CURRENT);
#ifdef LOW_BATTERY_PROTECT
		PMICLOG(" g_low_battery_level=%d,ret=%d,g_lowbat_int_bottom=%d\n", g_low_battery_level, ret,
			g_lowbat_int_bottom);
#endif
	}

	return ret;
}



int dlpt_notify_handler(void *unused)
{
	ktime_t ktime;
	int pre_ui_soc = 0;
	int cur_ui_soc = 0;
	int diff_ui_soc = 1;

#if defined(CONFIG_MTK_SMART_BATTERY)
	pre_ui_soc = bat_get_ui_percentage();
#endif
	cur_ui_soc = pre_ui_soc;

	do {
		ktime = ktime_set(10, 0);

		wait_event_interruptible(dlpt_notify_waiter, (dlpt_notify_flag == true));

#if !defined CONFIG_HAS_WAKELOCKS
		__pm_stay_awake(&dlpt_notify_lock);
#else
		wake_lock(&dlpt_notify_lock);
#endif
		mutex_lock(&dlpt_notify_mutex);
		/*---------------------------------*/

#if defined(CONFIG_MTK_SMART_BATTERY)
		cur_ui_soc = bat_get_ui_percentage();
#endif

		if (cur_ui_soc <= 1) {
			g_low_per_timer += 10;
			if (g_low_per_timer > g_low_per_timeout_val)
				g_low_per_timer = 0;
			PMICLOG("[DLPT] g_low_per_timer=%d,g_low_per_timeout_val=%d\n",
				g_low_per_timer, g_low_per_timeout_val);
		} else {
			g_low_per_timer = 0;
		}

		PMICLOG("[dlpt_notify_handler] %d %d %d %d %d\n", pre_ui_soc, cur_ui_soc,
			g_imix_val, g_low_per_timer, g_low_per_timeout_val);
/* can enable for ricky's request for >1% battery change TBD
	if( ((pre_ui_soc-cur_ui_soc)>=diff_ui_soc) ||
	    ((cur_ui_soc-pre_ui_soc)>=diff_ui_soc) ||
	    (g_imix_val == 0)                      ||
	    (g_imix_val == -1)                     ||
	    ( (cur_ui_soc <= 1) && (g_low_per_timer >= g_low_per_timeout_val) )

	  )
*/
		{

			PMICLOG("[DLPT] is running\n");
			if (ptim_rac_val_avg == 0)
				pr_err("[DLPT] ptim_rac_val_avg=0 , skip\n");
			else {
				if (upmu_get_rgs_chrdet()) {
					g_imix_val = get_dlpt_imix_charging();
				} else {
					g_imix_val = get_dlpt_imix();

/*
	if(g_imix_val != -1) {
		if(g_imix_val_pre <= 0)
			g_imix_val_pre=g_imix_val;

				if(g_imix_val > g_imix_val_pre) {
					PMICLOG("[DLPT] g_imix_val=%d,g_imix_val_pre=%d\n", g_imix_val, g_imix_val_pre);
					g_imix_val=g_imix_val_pre;
				}
				else
					g_imix_val_pre=g_imix_val;
				}
*/
				}

				/*Notify*/
				if (g_imix_val >= 1) {
					if (g_imix_val > IMAX_MAX_VALUE)
						g_imix_val = IMAX_MAX_VALUE;
					exec_dlpt_callback(g_imix_val);
				} else {
					exec_dlpt_callback(1);
					PMICLOG("[DLPT] return 1\n");
				}

				pre_ui_soc = cur_ui_soc;

				pr_err("[DLPT_final] %d,%d,%d,%d,%d,%d\n",
					g_imix_val, g_imix_val_pre, pre_ui_soc, cur_ui_soc,
					diff_ui_soc, IMAX_MAX_VALUE);
			}

		}

		g_dlpt_start = 1;
		dlpt_notify_flag = false;

		/*---------------------------------*/
		mutex_unlock(&dlpt_notify_mutex);
#if !defined CONFIG_HAS_WAKELOCKS
		__pm_relax(&dlpt_notify_lock);
#else
		wake_unlock(&dlpt_notify_lock);
#endif

		hrtimer_start(&dlpt_notify_timer, ktime, HRTIMER_MODE_REL);

	} while (!kthread_should_stop());

	return 0;
}

enum hrtimer_restart dlpt_notify_task(struct hrtimer *timer)
{
	dlpt_notify_flag = true;
	wake_up_interruptible(&dlpt_notify_waiter);
	PMICLOG("dlpt_notify_task is called\n");

	return HRTIMER_NORESTART;
}

int get_system_loading_ma(void)
{
	int fg_val = 0;

	if (g_dlpt_start == 0)
		PMICLOG("get_system_loading_ma not ready\n");
	else {
#if defined(CONFIG_MTK_SMART_BATTERY)
		fg_val = battery_meter_get_battery_current();
		fg_val = fg_val / 10;
		if (battery_meter_get_battery_current_sign() == 1)
			fg_val = 0 - fg_val;	/* charging*/
		PMICLOG("[get_system_loading_ma] fg_val=%d\n", fg_val);
#endif
	}

	return fg_val;
}


void dlpt_notify_init(void)
{
	ktime_t ktime;

	ktime = ktime_set(30, 0);
	hrtimer_init(&dlpt_notify_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	dlpt_notify_timer.function = dlpt_notify_task;
	hrtimer_start(&dlpt_notify_timer, ktime, HRTIMER_MODE_REL);

	dlpt_notify_thread = kthread_run(dlpt_notify_handler, 0, "dlpt_notify_thread");
	if (IS_ERR(dlpt_notify_thread))
		pr_err("Failed to create dlpt_notify_thread\n");
	else
		pr_err("Create dlpt_notify_thread : done\n");

	pmic_set_register_value(PMIC_RG_UVLO_VTHL, 0);

	/*re-init UVLO volt */
	switch (POWER_UVLO_VOLT_LEVEL) {
	case 2500:
		pmic_set_register_value(PMIC_RG_UVLO_VTHL, 0);
		break;
	case 2550:
		pmic_set_register_value(PMIC_RG_UVLO_VTHL, 1);
		break;
	case 2600:
		pmic_set_register_value(PMIC_RG_UVLO_VTHL, 2);
		break;
	case 2650:
		pmic_set_register_value(PMIC_RG_UVLO_VTHL, 3);
		break;
	case 2700:
		pmic_set_register_value(PMIC_RG_UVLO_VTHL, 4);
		break;
	case 2750:
		pmic_set_register_value(PMIC_RG_UVLO_VTHL, 5);
		break;
	case 2800:
		pmic_set_register_value(PMIC_RG_UVLO_VTHL, 6);
		break;
	case 2850:
		pmic_set_register_value(PMIC_RG_UVLO_VTHL, 7);
		break;
	case 2900:
		pmic_set_register_value(PMIC_RG_UVLO_VTHL, 8);
		break;
	default:
		PMICLOG("Invalid value(%d)\n", POWER_UVLO_VOLT_LEVEL);
		break;
	}
	pr_err("POWER_UVLO_VOLT_LEVEL=%d, [0x%x]=0x%x\n",
		POWER_UVLO_VOLT_LEVEL, MT6351_CHR_CON17, upmu_get_reg_value(MT6351_CHR_CON17));
}

#else
int get_dlpt_imix_spm(void)
{
	return 1;
}


#endif				/*#ifdef DLPT_FEATURE_SUPPORT */

int pmic_rdy = 0, usb_rdy = 0;
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
#if defined(CONFIG_MTK_FPGA)
	return 0;
#else
#if !defined CONFIG_MTK_LEGACY
	return gpiod_get_value(gpio_to_desc(130));
	/*return __gpio_get_value(130);*/
	/*return mt_get_gpio_in(130);*/
#else
	return 0;
#endif
#endif
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
/*
#ifdef MTK_BQ24261_SUPPORT
extern int is_bq24261_exist(void);
#endif

int is_ext_swchr_exist(void)
{
    #ifdef MTK_BQ24261_SUPPORT
	if (is_bq24261_exist() == 1)
		return 1;
	else
		return 0;
    #else
	PMICLOG("[is_ext_swchr_exist] no define any HW\n");
	return 0;
    #endif
}
*/

/*****************************************************************************
 * Enternal VBAT Boost status
 ******************************************************************************/
/*
extern int is_tps6128x_sw_ready(void);
extern int is_tps6128x_exist(void);

int is_ext_vbat_boost_sw_ready(void)
{
    if( (is_tps6128x_sw_ready()==1) )
	return 1;
    else
	return 0;
}

int is_ext_vbat_boost_exist(void)
{
    if( (is_tps6128x_exist()==1) )
	return 1;
    else
	return 0;
}

*/

/*****************************************************************************
 * Enternal BUCK status
 ******************************************************************************/

int get_ext_buck_i2c_ch_num(void)
{
	if (is_mt6311_exist() == 1)
		return get_mt6311_i2c_ch_num();
	else
		return -1;
}

int is_ext_buck_sw_ready(void)
{
	if ((is_mt6311_sw_ready() == 1))
		return 1;
	else
		return 0;
}

int is_ext_buck_exist(void)
{
	if ((is_mt6311_exist() == 1))
		return 1;
	else
		return 0;
}

/*****************************************************************************
 * FTM
 ******************************************************************************/
#define PMIC_DEVNAME "pmic_ftm"
#define Get_IS_EXT_BUCK_EXIST _IOW('k', 20, int)
#define Get_IS_EXT_VBAT_BOOST_EXIST _IOW('k', 21, int)
#define Get_IS_EXT_SWCHR_EXIST _IOW('k', 22, int)
#define Get_IS_EXT_BUCK2_EXIST _IOW('k', 23, int)


static struct class *pmic_class;
static struct cdev *pmic_cdev;
static int pmic_major;
static dev_t pmic_devno;

static long pmic_ftm_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int *user_data_addr;
	int ret = 0;
	int adc_in_data[2] = { 1, 1 };
	int adc_out_data[2] = { 1, 1 };

	switch (cmd) {
		/*#if defined(FTM_EXT_BUCK_CHECK)*/
	case Get_IS_EXT_BUCK_EXIST:
		user_data_addr = (int *)arg;
		ret = copy_from_user(adc_in_data, user_data_addr, 8);
		adc_out_data[0] = is_ext_buck_exist();
		ret = copy_to_user(user_data_addr, adc_out_data, 8);
		PMICLOG("[pmic_ftm_ioctl] Get_IS_EXT_BUCK_EXIST:%d\n", adc_out_data[0]);
		break;
		/*#endif*/

		/*#if defined(FTM_EXT_VBAT_BOOST_CHECK)*/
	case Get_IS_EXT_VBAT_BOOST_EXIST:
		user_data_addr = (int *)arg;
		ret = copy_from_user(adc_in_data, user_data_addr, 8);
		adc_out_data[0] = is_ext_vbat_boost_exist();
		ret = copy_to_user(user_data_addr, adc_out_data, 8);
		PMICLOG("[pmic_ftm_ioctl] Get_IS_EXT_VBAT_BOOST_EXIST:%d\n", adc_out_data[0]);
		break;
		/*#endif*/

		/*#if defined(FEATURE_FTM_SWCHR_HW_DETECT)*/
	case Get_IS_EXT_SWCHR_EXIST:
		user_data_addr = (int *)arg;
		ret = copy_from_user(adc_in_data, user_data_addr, 8);
		adc_out_data[0] = is_ext_swchr_exist();
		ret = copy_to_user(user_data_addr, adc_out_data, 8);
		PMICLOG("[pmic_ftm_ioctl] Get_IS_EXT_SWCHR_EXIST:%d\n", adc_out_data[0]);
		break;
		/*#endif*/
	case Get_IS_EXT_BUCK2_EXIST:
		user_data_addr = (int *)arg;
		ret = copy_from_user(adc_in_data, user_data_addr, 8);
		adc_out_data[0] = is_ext_buck2_exist();
		ret = copy_to_user(user_data_addr, adc_out_data, 8);
		PMICLOG("[pmic_ftm_ioctl] Get_IS_EXT_BUCK2_EXIST:%d\n", adc_out_data[0]);
		break;
	default:
		PMICLOG("[pmic_ftm_ioctl] Error ID\n");
		break;
	}

	return 0;
}
#ifdef CONFIG_COMPAT
static long pmic_ftm_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
		/*#if defined(FTM_EXT_BUCK_CHECK) */
	case Get_IS_EXT_BUCK_EXIST:
	case Get_IS_EXT_VBAT_BOOST_EXIST:
	case Get_IS_EXT_SWCHR_EXIST:
	case Get_IS_EXT_BUCK2_EXIST:
		ret = file->f_op->unlocked_ioctl(file, cmd, arg);
		break;
	default:
		PMICLOG("[pmic_ftm_compat_ioctl] Error ID\n");
		break;
	}

	return 0;
}
#endif
static int pmic_ftm_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int pmic_ftm_release(struct inode *inode, struct file *file)
{
	return 0;
}


static const struct file_operations pmic_ftm_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = pmic_ftm_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = pmic_ftm_compat_ioctl,
#endif
	.open = pmic_ftm_open,
	.release = pmic_ftm_release,
};

void pmic_ftm_init(void)
{
	struct class_device *class_dev = NULL;
	int ret = 0;

	ret = alloc_chrdev_region(&pmic_devno, 0, 1, PMIC_DEVNAME);
	if (ret)
		PMICLOG("[pmic_ftm_init] Error: Can't Get Major number for pmic_ftm\n");

	pmic_cdev = cdev_alloc();
	pmic_cdev->owner = THIS_MODULE;
	pmic_cdev->ops = &pmic_ftm_fops;

	ret = cdev_add(pmic_cdev, pmic_devno, 1);
	if (ret)
		PMICLOG("[pmic_ftm_init] Error: cdev_add\n");

	pmic_major = MAJOR(pmic_devno);
	pmic_class = class_create(THIS_MODULE, PMIC_DEVNAME);

	class_dev = (struct class_device *)device_create(pmic_class,
							 NULL, pmic_devno, NULL, PMIC_DEVNAME);

	PMICLOG("[pmic_ftm_init] Done\n");
}




/*****************************************************************************
 * HW Setting
 ******************************************************************************/
unsigned short is_battery_remove = 0;
unsigned short is_wdt_reboot_pmic = 0;
unsigned short is_wdt_reboot_pmic_chk = 0;

unsigned short is_battery_remove_pmic(void)
{
	return is_battery_remove;
}

void PMIC_CUSTOM_SETTING_V1(void)
{
#if 0
#if defined CONFIG_MTK_LEGACY
#if defined(CONFIG_MTK_FPGA)
#else
	pmu_drv_tool_customization_init();	/* legacy DCT only */
#endif
#endif
#endif
}


/*****************************************************************************
 * Dump all LDO status
 ******************************************************************************/
void dump_ldo_status_read_debug(void)
{
	int i, j;
	int en = 0;
	int voltage_reg = 0;
	int voltage = 0;
	const int *pVoltage;

	pr_debug("********** BUCK/LDO status dump [1:ON,0:OFF]**********\n");

	for (i = 0; i < ARRAY_SIZE(mtk_bucks); i++) {
		if (mtk_bucks[i].qi_en_reg != 0)
			en = pmic_get_register_value(mtk_bucks[i].qi_en_reg);
		else
			en = -1;

		if (mtk_bucks[i].qi_vol_reg != 0) {
			voltage_reg = pmic_get_register_value(mtk_bucks[i].qi_vol_reg);
			voltage =
			    mtk_bucks[i].desc.min_uV + mtk_bucks[i].desc.uV_step * voltage_reg;
		} else {
			voltage_reg = -1;
			voltage = -1;
		}
		pr_err("%s   status:%d     voltage:%duv    voltage_reg:%d\n",
			mtk_bucks[i].desc.name, en, voltage, voltage_reg);
	}

	for (i = 0; i < ARRAY_SIZE(mtk_ldos); i++) {
		if (mtk_ldos[i].en_reg != 0)
			en = pmic_get_register_value(mtk_ldos[i].en_reg);
		else
			en = -1;

		if (mtk_ldos[i].desc.n_voltages != 1) {
			if (mtk_ldos[i].vol_reg != 0) {
				voltage_reg = pmic_get_register_value(mtk_ldos[i].vol_reg);
				if (mtk_ldos[i].pvoltages != NULL) {
					pVoltage = (const int *)mtk_ldos[i].pvoltages;
					/*HW LDO sequence issue, we need to change it */
					if ((strcmp(mtk_ldos[i].desc.name, "va18") == 0) |
						(strcmp(mtk_ldos[i].desc.name, "vtcxo24") == 0) |
						(strcmp(mtk_ldos[i].desc.name, "vtcxo28") == 0) |
						(strcmp(mtk_ldos[i].desc.name, "vcn28") == 0) |
						(strcmp(mtk_ldos[i].desc.name, "vxo22") == 0) |
						(strcmp(mtk_ldos[i].desc.name, "vbif28") == 0)) {
						pr_err("dump_ldo_status_read_debug(name=%s selector=%d)\n",
						mtk_ldos[i].desc.name, voltage_reg);
					if (voltage_reg == 0)
						voltage_reg = 3;
					else if  (voltage_reg == 1)
						voltage_reg = 2;
					else if  (voltage_reg == 2)
						voltage_reg = 1;
					else if  (voltage_reg == 3)
						voltage_reg = 0;
				}
					voltage = pVoltage[voltage_reg];
				} else {
					voltage =
					    mtk_ldos[i].desc.min_uV +
					    mtk_ldos[i].desc.uV_step * voltage_reg;
				}
			} else {
				voltage_reg = -1;
				voltage = -1;
			}
		} else {
			pVoltage = (const int *)mtk_ldos[i].pvoltages;
			voltage = pVoltage[0];
		}

		pr_err("%s   status:%d     voltage:%duv    voltage_reg:%d\n",
			mtk_ldos[i].desc.name, en, voltage, voltage_reg);
	}

	for (i = 0; i < interrupts_size; i++) {
		for (j = 0; j < PMIC_INT_WIDTH; j++) {

			PMICLOG("[PMIC_INT][%s] interrupt issue times: %d\n",
				interrupts[i].interrupts[j].name,
				interrupts[i].interrupts[j].times);
		}
	}

	PMICLOG("Power Good Status=0x%x.\n", upmu_get_reg_value(0x21c));
	PMICLOG("OC Status=0x%x.\n", upmu_get_reg_value(0x214));
	PMICLOG("Thermal Status=0x%x.\n", upmu_get_reg_value(0x21e));
}

static int proc_utilization_show(struct seq_file *m, void *v)
{
	int i, j;
	int en = 0;
	int voltage_reg = 0;
	int voltage = 0;
	const int *pVoltage;

	seq_puts(m, "********** BUCK/LDO status dump [1:ON,0:OFF]**********\n");

	for (i = 0; i < ARRAY_SIZE(mtk_bucks); i++) {
		if (mtk_bucks[i].qi_en_reg != 0)
			en = pmic_get_register_value(mtk_bucks[i].qi_en_reg);
		else
			en = -1;

		if (mtk_bucks[i].qi_vol_reg != 0) {
			voltage_reg = pmic_get_register_value(mtk_bucks[i].qi_vol_reg);
			voltage =
			    mtk_bucks[i].desc.min_uV + mtk_bucks[i].desc.uV_step * voltage_reg;
		} else {
			voltage_reg = -1;
			voltage = -1;
		}
		seq_printf(m, "%s   status:%d     voltage:%duv    voltage_reg:%d\n",
			   mtk_bucks[i].desc.name, en, voltage, voltage_reg);
	}

	for (i = 0; i < ARRAY_SIZE(mtk_ldos); i++) {
		if (mtk_ldos[i].en_reg != 0)
			en = pmic_get_register_value(mtk_ldos[i].en_reg);
		else
			en = -1;

		if (mtk_ldos[i].desc.n_voltages != 1) {
			if (mtk_ldos[i].vol_reg != 0) {
				voltage_reg = pmic_get_register_value(mtk_ldos[i].vol_reg);
				if (mtk_ldos[i].pvoltages != NULL) {
					pVoltage = (const int *)mtk_ldos[i].pvoltages;
					/*HW LDO sequence issue, we need to change it */
					if ((strcmp(mtk_ldos[i].desc.name, "va18") == 0) |
						(strcmp(mtk_ldos[i].desc.name, "vtcxo24") == 0) |
						(strcmp(mtk_ldos[i].desc.name, "vtcxo28") == 0) |
						(strcmp(mtk_ldos[i].desc.name, "vcn28") == 0) |
						(strcmp(mtk_ldos[i].desc.name, "vxo22") == 0) |
						(strcmp(mtk_ldos[i].desc.name, "vbif28") == 0)) {
						pr_err("dump_ldo_status_read_debug(name=%s selector=%d)\n",
							mtk_ldos[i].desc.name, voltage_reg);
					if (voltage_reg == 0)
						voltage_reg = 3;
					else if  (voltage_reg == 1)
						voltage_reg = 2;
					else if  (voltage_reg == 2)
						voltage_reg = 1;
					else if  (voltage_reg == 3)
						voltage_reg = 0;
					}
					voltage = pVoltage[voltage_reg];
				} else {
					voltage =
					    mtk_ldos[i].desc.min_uV +
					    mtk_ldos[i].desc.uV_step * voltage_reg;
				}
			} else {
				voltage_reg = -1;
				voltage = -1;
			}
		} else {
			pVoltage = (const int *)mtk_ldos[i].pvoltages;
			voltage = pVoltage[0];
		}
		seq_printf(m, "%s   status:%d     voltage:%duv    voltage_reg:%d\n",
			   mtk_ldos[i].desc.name, en, voltage, voltage_reg);
	}

	for (i = 0; i < interrupts_size; i++) {
		for (j = 0; j < PMIC_INT_WIDTH; j++) {

			seq_printf(m, "[PMIC_INT][%s] interrupt issue times: %d\n",
				   interrupts[i].interrupts[j].name,
				   interrupts[i].interrupts[j].times);
		}
	}
	seq_printf(m, "Power Good Status=0x%x.\n", upmu_get_reg_value(0x21c));
	seq_printf(m, "OC Status=0x%x.\n", upmu_get_reg_value(0x214));
	seq_printf(m, "Thermal Status=0x%x.\n", upmu_get_reg_value(0x21e));

	return 0;
}

static int proc_utilization_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_utilization_show, NULL);
}

static const struct file_operations pmic_debug_proc_fops = {
	.open = proc_utilization_open,
	.read = seq_read,
};

static int proc_dump_register_show(struct seq_file *m, void *v)
{
	int i;

	seq_puts(m, "********** dump PMIC registers**********\n");

	for (i = 0; i <= 0x0fae; i = i + 10) {
		seq_printf(m, "Reg[%x]=0x%x Reg[%x]=0x%x Reg[%x]=0x%x Reg[%x]=0x%x Reg[%x]=0x%x\n",
			i, upmu_get_reg_value(i), i + 1, upmu_get_reg_value(i + 1), i + 2,
			upmu_get_reg_value(i + 2), i + 3, upmu_get_reg_value(i + 3), i + 4,
			upmu_get_reg_value(i + 4));

		seq_printf(m, "Reg[%x]=0x%x Reg[%x]=0x%x Reg[%x]=0x%x Reg[%x]=0x%x Reg[%x]=0x%x\n",
			i + 5, upmu_get_reg_value(i + 5), i + 6, upmu_get_reg_value(i + 6),
			i + 7, upmu_get_reg_value(i + 7), i + 8, upmu_get_reg_value(i + 8),
			i + 9, upmu_get_reg_value(i + 9));
	}

	return 0;
}

static int proc_dump_register_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_dump_register_show, NULL);
}

static const struct file_operations pmic_dump_register_proc_fops = {
	.open = proc_dump_register_open,
	.read = seq_read,
};

void pmic_debug_init(void)
{
	struct proc_dir_entry *mt_pmic_dir;

	mt_pmic_dir = proc_mkdir("mt_pmic", NULL);
	if (!mt_pmic_dir) {
		PMICLOG("fail to mkdir /proc/mt_pmic\n");
		return;
	}

	proc_create("dump_ldo_status", S_IRUGO | S_IWUSR, mt_pmic_dir, &pmic_debug_proc_fops);
	PMICLOG("proc_create pmic_debug_proc_fops\n");

	proc_create("dump_pmic_reg", S_IRUGO | S_IWUSR, mt_pmic_dir, &pmic_dump_register_proc_fops);
	PMICLOG("proc_create pmic_dump_register_proc_fops\n");



}

static bool pwrkey_detect_flag;
static struct hrtimer pwrkey_detect_timer;
static struct task_struct *pwrkey_detect_thread;
static DECLARE_WAIT_QUEUE_HEAD(pwrkey_detect_waiter);

#define BAT_MS_TO_NS(x) (x * 1000 * 1000)

enum hrtimer_restart pwrkey_detect_sw_workaround(struct hrtimer *timer)
{
	pwrkey_detect_flag = true;

	wake_up_interruptible(&pwrkey_detect_waiter);
	return HRTIMER_NORESTART;
}

int pwrkey_detect_sw_thread_handler(void *unused)
{
	ktime_t ktime;

	do {
		ktime = ktime_set(3, BAT_MS_TO_NS(1000));



		wait_event_interruptible(pwrkey_detect_waiter, (pwrkey_detect_flag == true));

		/*PMICLOG("=>charger_hv_detect_sw_workaround\n"); */
		if (pmic_get_register_value(PMIC_RG_STRUP_75K_CK_PDN) == 1) {
			PMICLOG("charger_hv_detect_sw_workaround =0x%x\n",
				upmu_get_reg_value(0x24e));
			pmic_set_register_value(PMIC_RG_STRUP_75K_CK_PDN, 0);
		}


		hrtimer_start(&pwrkey_detect_timer, ktime, HRTIMER_MODE_REL);

	} while (!kthread_should_stop());

	return 0;

}



void pwrkey_sw_workaround_init(void)
{
	ktime_t ktime;

	ktime = ktime_set(0, BAT_MS_TO_NS(2000));
	hrtimer_init(&pwrkey_detect_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	pwrkey_detect_timer.function = pwrkey_detect_sw_workaround;
	hrtimer_start(&pwrkey_detect_timer, ktime, HRTIMER_MODE_REL);

	pwrkey_detect_thread =
	    kthread_run(pwrkey_detect_sw_thread_handler, 0, "mtk pwrkey_sw_workaround_init");

	if (IS_ERR(pwrkey_detect_thread))
		PMICLOG("[%s]: failed to create pwrkey_detect_thread thread\n", __func__);

}

#ifdef LOW_BATTERY_PROTECT
/*****************************************************************************
 * low battery protect UT
 ******************************************************************************/
static ssize_t show_low_battery_protect_ut(struct device *dev, struct device_attribute *attr,
					   char *buf)
{
	PMICLOG("[show_low_battery_protect_ut] g_low_battery_level=%d\n", g_low_battery_level);
	return sprintf(buf, "%u\n", g_low_battery_level);
}

static ssize_t store_low_battery_protect_ut(struct device *dev, struct device_attribute *attr,
					    const char *buf, size_t size)
{
	int ret = 0;
	char *pvalue = NULL;
	unsigned int val = 0;

	pr_err("[store_low_battery_protect_ut]\n");

	if (buf != NULL && size != 0) {
		pr_err("[store_low_battery_protect_ut] buf is %s and size is %zu\n", buf, size);
		/*val = simple_strtoul(buf, &pvalue, 16);*/
		pvalue = (char *)buf;
		ret = kstrtou32(pvalue, 16, (unsigned int *)&val);
		if (val <= 2) {
			pr_err("[store_low_battery_protect_ut] your input is %d\n", val);
			exec_low_battery_callback(val);
		} else {
			pr_err("[store_low_battery_protect_ut] wrong number (%d)\n", val);
		}
	}
	return size;
}

static DEVICE_ATTR(low_battery_protect_ut, 0664, show_low_battery_protect_ut, store_low_battery_protect_ut);	/*664*/

/*****************************************************************************
 * low battery protect stop
 ******************************************************************************/
static ssize_t show_low_battery_protect_stop(struct device *dev, struct device_attribute *attr,
					     char *buf)
{
	PMICLOG("[show_low_battery_protect_stop] g_low_battery_stop=%d\n", g_low_battery_stop);
	return sprintf(buf, "%u\n", g_low_battery_stop);
}

static ssize_t store_low_battery_protect_stop(struct device *dev, struct device_attribute *attr,
					      const char *buf, size_t size)
{
	int ret = 0;
	char *pvalue = NULL;
	unsigned int val = 0;

	pr_err("[store_low_battery_protect_stop]\n");

	if (buf != NULL && size != 0) {
		pr_err("[store_low_battery_protect_stop] buf is %s and size is %zu\n", buf, size);
		/*val = simple_strtoul(buf, &pvalue, 16);*/
		pvalue = (char *)buf;
		ret = kstrtou32(pvalue, 16, (unsigned int *)&val);
		if ((val != 0) && (val != 1))
			val = 0;
		g_low_battery_stop = val;
		pr_err("[store_low_battery_protect_stop] g_low_battery_stop=%d\n",
			g_low_battery_stop);
	}
	return size;
}

static DEVICE_ATTR(low_battery_protect_stop, 0664,
			show_low_battery_protect_stop, store_low_battery_protect_stop);	/*664*/

/*
 * low battery protect level
 */
static ssize_t show_low_battery_protect_level(struct device *dev, struct device_attribute *attr,
					      char *buf)
{
	PMICLOG("[show_low_battery_protect_level] g_low_battery_level=%d\n", g_low_battery_level);
	return sprintf(buf, "%u\n", g_low_battery_level);
}

static ssize_t store_low_battery_protect_level(struct device *dev, struct device_attribute *attr,
					       const char *buf, size_t size)
{
	PMICLOG("[store_low_battery_protect_level] g_low_battery_level=%d\n", g_low_battery_level);

	return size;
}

static DEVICE_ATTR(low_battery_protect_level, 0664,
			show_low_battery_protect_level, store_low_battery_protect_level);	/*664*/
#endif

#ifdef BATTERY_OC_PROTECT
/*****************************************************************************
 * battery OC protect UT
 ******************************************************************************/
static ssize_t show_battery_oc_protect_ut(struct device *dev, struct device_attribute *attr,
					  char *buf)
{
	PMICLOG("[show_battery_oc_protect_ut] g_battery_oc_level=%d\n", g_battery_oc_level);
	return sprintf(buf, "%u\n", g_battery_oc_level);
}

static ssize_t store_battery_oc_protect_ut(struct device *dev, struct device_attribute *attr,
					   const char *buf, size_t size)
{
	int ret = 0;
	char *pvalue = NULL;
	unsigned int val = 0;

	pr_err("[store_battery_oc_protect_ut]\n");

	if (buf != NULL && size != 0) {
		pr_err("[store_battery_oc_protect_ut] buf is %s and size is %zu\n", buf, size);
		/*val = simple_strtoul(buf, &pvalue, 16);*/
		pvalue = (char *)buf;
		ret = kstrtou32(pvalue, 16, (unsigned int *)&val);
		if (val <= 1) {
			pr_err("[store_battery_oc_protect_ut] your input is %d\n", val);
			exec_battery_oc_callback(val);
		} else {
			pr_err("[store_battery_oc_protect_ut] wrong number (%d)\n", val);
		}
	}
	return size;
}

static DEVICE_ATTR(battery_oc_protect_ut, 0664,
			show_battery_oc_protect_ut, store_battery_oc_protect_ut);	/*664*/

/*****************************************************************************
 * battery OC protect stop
 ******************************************************************************/
static ssize_t show_battery_oc_protect_stop(struct device *dev, struct device_attribute *attr,
					    char *buf)
{
	PMICLOG("[show_battery_oc_protect_stop] g_battery_oc_stop=%d\n", g_battery_oc_stop);
	return sprintf(buf, "%u\n", g_battery_oc_stop);
}

static ssize_t store_battery_oc_protect_stop(struct device *dev, struct device_attribute *attr,
					     const char *buf, size_t size)
{
	int ret = 0;
	char *pvalue = NULL;
	unsigned int val = 0;

	pr_err("[store_battery_oc_protect_stop]\n");

	if (buf != NULL && size != 0) {
		pr_err("[store_battery_oc_protect_stop] buf is %s and size is %zu\n", buf, size);
		/*val = simple_strtoul(buf, &pvalue, 16);*/
		pvalue = (char *)buf;
		ret = kstrtou32(pvalue, 16, (unsigned int *)&val);
		if ((val != 0) && (val != 1))
			val = 0;
		g_battery_oc_stop = val;
		pr_err("[store_battery_oc_protect_stop] g_battery_oc_stop=%d\n",
			g_battery_oc_stop);
	}
	return size;
}

static DEVICE_ATTR(battery_oc_protect_stop, 0664,
			show_battery_oc_protect_stop, store_battery_oc_protect_stop);	/*664*/

/*****************************************************************************
 * battery OC protect level
 ******************************************************************************/
static ssize_t show_battery_oc_protect_level(struct device *dev, struct device_attribute *attr,
					     char *buf)
{
	PMICLOG("[show_battery_oc_protect_level] g_battery_oc_level=%d\n", g_battery_oc_level);
	return sprintf(buf, "%u\n", g_battery_oc_level);
}

static ssize_t store_battery_oc_protect_level(struct device *dev, struct device_attribute *attr,
					      const char *buf, size_t size)
{
	PMICLOG("[store_battery_oc_protect_level] g_battery_oc_level=%d\n", g_battery_oc_level);

	return size;
}

static DEVICE_ATTR(battery_oc_protect_level, 0664,
			show_battery_oc_protect_level, store_battery_oc_protect_level);	/*664*/
#endif

#ifdef BATTERY_PERCENT_PROTECT
/*****************************************************************************
 * battery percent protect UT
 ******************************************************************************/
static ssize_t show_battery_percent_ut(struct device *dev, struct device_attribute *attr,
					       char *buf)
{
	/*show_battery_percent_protect_ut*/
	PMICLOG("[show_battery_percent_protect_ut] g_battery_percent_level=%d\n",
		g_battery_percent_level);
	return sprintf(buf, "%u\n", g_battery_percent_level);
}

static ssize_t store_battery_percent_ut(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t size)
{
	int ret = 0;
	char *pvalue = NULL;
	unsigned int val = 0;
	/*store_battery_percent_protect_ut*/
	pr_err("[store_battery_percent_protect_ut]\n");

	if (buf != NULL && size != 0) {
		pr_err("[store_battery_percent_protect_ut] buf is %s and size is %zu\n", buf,
			size);
		/*val = simple_strtoul(buf, &pvalue, 16);*/
		pvalue = (char *)buf;
		ret = kstrtou32(pvalue, 16, (unsigned int *)&val);
		if (val <= 1) {
			pr_err("[store_battery_percent_protect_ut] your input is %d\n", val);
			exec_battery_percent_callback(val);
		} else {
			pr_err("[store_battery_percent_protect_ut] wrong number (%d)\n", val);
		}
	}
	return size;
}

static DEVICE_ATTR(battery_percent_protect_ut, 0664,
			show_battery_percent_ut, store_battery_percent_ut);	/*664*/

/*
 * battery percent protect stop
 ******************************************************************************/
static ssize_t show_battery_percent_stop(struct device *dev, struct device_attribute *attr,
						 char *buf)
{
	/*show_battery_percent_protect_stop*/
	PMICLOG("[show_battery_percent_protect_stop] g_battery_percent_stop=%d\n",
		g_battery_percent_stop);
	return sprintf(buf, "%u\n", g_battery_percent_stop);
}

static ssize_t store_battery_percent_stop(struct device *dev, struct device_attribute *attr,
						  const char *buf, size_t size)
{
	int ret = 0;
	char *pvalue = NULL;
	unsigned int val = 0;
	/*store_battery_percent_protect_stop*/
	pr_err("[store_battery_percent_protect_stop]\n");

	if (buf != NULL && size != 0) {
		pr_err("[store_battery_percent_protect_stop] buf is %s and size is %zu\n", buf,
			size);
		/*val = simple_strtoul(buf, &pvalue, 16);*/
		pvalue = (char *)buf;
		ret = kstrtou32(pvalue, 16, (unsigned int *)&val);
		if ((val != 0) && (val != 1))
			val = 0;
		g_battery_percent_stop = val;
		pr_err("[store_battery_percent_protect_stop] g_battery_percent_stop=%d\n",
			g_battery_percent_stop);
	}
	return size;
}

static DEVICE_ATTR(battery_percent_protect_stop, 0664,
			show_battery_percent_stop, store_battery_percent_stop);	/*664*/

/*
 * battery percent protect level
 ******************************************************************************/
static ssize_t show_battery_percent_level(struct device *dev, struct device_attribute *attr,
						  char *buf)
{
	/*show_battery_percent_protect_level*/
	PMICLOG("[show_battery_percent_protect_level] g_battery_percent_level=%d\n",
		g_battery_percent_level);
	return sprintf(buf, "%u\n", g_battery_percent_level);
}

static ssize_t store_battery_percent_level(struct device *dev,
						   struct device_attribute *attr, const char *buf,
						   size_t size)
{
	/*store_battery_percent_protect_level*/
	PMICLOG("[store_battery_percent_protect_level] g_battery_percent_level=%d\n",
		g_battery_percent_level);

	return size;
}

static DEVICE_ATTR(battery_percent_protect_level, 0664,
			show_battery_percent_level, store_battery_percent_level);	/*664*/
#endif

#ifdef DLPT_FEATURE_SUPPORT
/*****************************************************************************
 * DLPT UT
 ******************************************************************************/
static ssize_t show_dlpt_ut(struct device *dev, struct device_attribute *attr, char *buf)
{
	PMICLOG("[show_dlpt_ut] g_dlpt_val=%d\n", g_dlpt_val);
	return sprintf(buf, "%u\n", g_dlpt_val);
}

static ssize_t store_dlpt_ut(struct device *dev, struct device_attribute *attr, const char *buf,
			     size_t size)
{
	char *pvalue = NULL;
	unsigned int val = 0;
	int ret = 0;

	pr_err("[store_dlpt_ut]\n");

	if (buf != NULL && size != 0) {
		pr_err("[store_dlpt_ut] buf is %s and size is %zu\n", buf, size);
		/*val = simple_strtoul(buf, &pvalue, 10);*/
		pvalue = (char *)buf;
		ret = kstrtou32(pvalue, 10, (unsigned int *)&val);

		pr_err("[store_dlpt_ut] your input is %d\n", val);
		exec_dlpt_callback(val);
	}
	return size;
}

static DEVICE_ATTR(dlpt_ut, 0664, show_dlpt_ut, store_dlpt_ut);	/*664*/

/*****************************************************************************
 * DLPT stop
 ******************************************************************************/
static ssize_t show_dlpt_stop(struct device *dev, struct device_attribute *attr, char *buf)
{
	PMICLOG("[show_dlpt_stop] g_dlpt_stop=%d\n", g_dlpt_stop);
	return sprintf(buf, "%u\n", g_dlpt_stop);
}

static ssize_t store_dlpt_stop(struct device *dev, struct device_attribute *attr, const char *buf,
			       size_t size)
{
	char *pvalue = NULL;
	unsigned int val = 0;
	int ret = 0;

	pr_err("[store_dlpt_stop]\n");

	if (buf != NULL && size != 0) {
		pr_err("[store_dlpt_stop] buf is %s and size is %zu\n", buf, size);
		/*val = simple_strtoul(buf, &pvalue, 16);*/
		pvalue = (char *)buf;
		ret = kstrtou32(pvalue, 16, (unsigned int *)&val);
		if ((val != 0) && (val != 1))
			val = 0;
		g_dlpt_stop = val;
		pr_err("[store_dlpt_stop] g_dlpt_stop=%d\n", g_dlpt_stop);
	}
	return size;
}

static DEVICE_ATTR(dlpt_stop, 0664, show_dlpt_stop, store_dlpt_stop);	/*664 */

/*****************************************************************************
 * DLPT level
 ******************************************************************************/
static ssize_t show_dlpt_level(struct device *dev, struct device_attribute *attr, char *buf)
{
	PMICLOG("[show_dlpt_level] g_dlpt_val=%d\n", g_dlpt_val);
	return sprintf(buf, "%u\n", g_dlpt_val);
}

static ssize_t store_dlpt_level(struct device *dev, struct device_attribute *attr, const char *buf,
				size_t size)
{
	PMICLOG("[store_dlpt_level] g_dlpt_val=%d\n", g_dlpt_val);

	return size;
}

static DEVICE_ATTR(dlpt_level, 0664, show_dlpt_level, store_dlpt_level);	/*664*/
#endif

/*****************************************************************************
 * system function
 ******************************************************************************/
#ifdef DLPT_FEATURE_SUPPORT
static unsigned long pmic_node;

static int fb_early_init_dt_get_chosen(unsigned long node, const char *uname, int depth, void *data)
{
	if (depth != 1 || (strcmp(uname, "chosen") != 0 && strcmp(uname, "chosen@0") != 0))
		return 0;
	pmic_node = node;
	return 1;
}
#endif /*end of #ifdef DLPT_FEATURE_SUPPORT*/
static int __init pmic_mt_probe(struct platform_device *dev)
{
	int ret_device_file = 0, i;
#ifdef DLPT_FEATURE_SUPPORT
	const int *pimix = NULL;
	int len = 0;
#endif
#if defined(CONFIG_MTK_SMART_BATTERY)
	struct device_node *np;
	u32 val;
	char *path = "/bus/BAT_METTER";

	np = of_find_node_by_path(path);
	if (of_property_read_u32(np, "car_tune_value", &val) == 0) {
		batt_meter_cust_data.car_tune_value = (int)val;
		PMICLOG("Get car_tune_value from DT: %d\n",
			 batt_meter_cust_data.car_tune_value);
	} else {
		batt_meter_cust_data.car_tune_value = CAR_TUNE_VALUE;
		PMICLOG("Get car_tune_value from cust header\n");
	}
#endif
	/*--- initailize pmic_suspend_state ---*/
	pmic_suspend_state = false;
#ifdef DLPT_FEATURE_SUPPORT
	if (of_scan_flat_dt(fb_early_init_dt_get_chosen, NULL) > 0)
		pimix = of_get_flat_dt_prop(pmic_node, "atag,imix_r", &len);
	if (pimix == NULL) {
		pr_err(" pimix==NULL len=%d\n", len);
	} else {
		pr_err(" pimix=%d\n", *pimix);
		ptim_rac_val_avg = *pimix;
	}

	PMICLOG("******** MT pmic driver probe!! ********%d\n", ptim_rac_val_avg);
	pr_debug("[PMIC]pmic_mt_probe %s %s\n", dev->name, dev->id_entry->name);
#endif /* #ifdef DLPT_FEATURE_SUPPORT */
	/*get PMIC CID */
	pr_debug
	    ("PMIC CID=0x%x PowerGoodStatus = 0x%x OCStatus = 0x%x ThermalStatus = 0x%x rsvStatus = 0x%x\n",
	     pmic_get_register_value(PMIC_SWCID), upmu_get_reg_value(0x21c),
	     upmu_get_reg_value(0x214), upmu_get_reg_value(0x21e), upmu_get_reg_value(0x2a6));

	/* upmu_set_reg_value(0x2a6, 0xff); */ /* TBD */

	/*pmic initial setting */
#if 0
	PMIC_INIT_SETTING_V1();
	PMICLOG("[PMIC_INIT_SETTING_V1] Done\n");
#else
	PMICLOG("[PMIC_INIT_SETTING_V1] delay to MT6311 init\n");
#endif
#if !defined CONFIG_MTK_LEGACY
/*      replace by DTS*/
#else
	PMIC_CUSTOM_SETTING_V1();
	PMICLOG("[PMIC_CUSTOM_SETTING_V1] Done\n");
#endif				/*End of #if !defined CONFIG_MTK_LEGACY */


/*#if defined(CONFIG_MTK_FPGA)*/
#if 0
	PMICLOG("[PMIC_EINT_SETTING] disable when CONFIG_MTK_FPGA\n");
#else
	/*PMIC Interrupt Service*/
	pmic_thread_handle = kthread_create(pmic_thread_kthread, (void *)NULL, "pmic_thread");
	if (IS_ERR(pmic_thread_handle)) {
		pmic_thread_handle = NULL;
		PMICLOG("[pmic_thread_kthread_mt6325] creation fails\n");
	} else {
		PMICLOG("[pmic_thread_kthread_mt6325] kthread_create Done\n");
	}

	PMIC_EINT_SETTING();
	PMICLOG("[PMIC_EINT_SETTING] Done\n");
#endif


	mtk_regulator_init(dev);

#ifdef LOW_BATTERY_PROTECT
	low_battery_protect_init();
#else
	pr_err("[PMIC] no define LOW_BATTERY_PROTECT\n");
#endif

#ifdef BATTERY_OC_PROTECT
	battery_oc_protect_init();
#else
	pr_err("[PMIC] no define BATTERY_OC_PROTECT\n");
#endif

#ifdef BATTERY_PERCENT_PROTECT
	bat_percent_notify_init();
#else
	pr_err("[PMIC] no define BATTERY_PERCENT_PROTECT\n");
#endif

#ifdef DLPT_FEATURE_SUPPORT
	dlpt_notify_init();
#else
	pr_err("[PMIC] no define DLPT_FEATURE_SUPPORT\n");
#endif

#if 0 /* jade do not use 26MHz for auxadc */
	pmic_set_register_value(PMIC_AUXADC_CK_AON, 1);
	pmic_set_register_value(PMIC_RG_CLKSQ_EN_AUX_AP_MODE, 0);
	PMICLOG("[PMIC] auxadc 26M test : Reg[0x%x]=0x%x, Reg[0x%x]=0x%x\n",
		MT6351_AUXADC_CON0, upmu_get_reg_value(MT6351_AUXADC_CON0),
		MT6351_TOP_CLKSQ, upmu_get_reg_value(MT6351_TOP_CLKSQ)
	    );
#endif

	pmic_debug_init();
	PMICLOG("[PMIC] pmic_debug_init : done.\n");

	pmic_ftm_init();



	/*EM BUCK voltage & Status*/
	for (i = 0; i < ARRAY_SIZE(mtk_bucks); i++) {
		/*PMICLOG("[PMIC] register buck id=%d\n",i);*/
		ret_device_file = device_create_file(&(dev->dev), &mtk_bucks[i].en_att);
		ret_device_file = device_create_file(&(dev->dev), &mtk_bucks[i].voltage_att);
		mtk_bucks[i].vsleep_en_saved = 0x0fffffff;
	}

	/*EM ldo voltage & Status*/
	for (i = 0; i < ARRAY_SIZE(mtk_ldos); i++) {
		/*PMICLOG("[PMIC] register ldo id=%d\n",i);*/
		ret_device_file = device_create_file(&(dev->dev), &mtk_ldos[i].en_att);
		ret_device_file = device_create_file(&(dev->dev), &mtk_ldos[i].voltage_att);
	}

	ret_device_file = device_create_file(&(dev->dev), &dev_attr_pmic_access);
	ret_device_file = device_create_file(&(dev->dev), &dev_attr_pmic_dvt);

#ifdef LOW_BATTERY_PROTECT
	ret_device_file = device_create_file(&(dev->dev), &dev_attr_low_battery_protect_ut);
	ret_device_file = device_create_file(&(dev->dev), &dev_attr_low_battery_protect_stop);
	ret_device_file = device_create_file(&(dev->dev), &dev_attr_low_battery_protect_level);
#endif

#ifdef BATTERY_OC_PROTECT
	ret_device_file = device_create_file(&(dev->dev), &dev_attr_battery_oc_protect_ut);
	ret_device_file = device_create_file(&(dev->dev), &dev_attr_battery_oc_protect_stop);
	ret_device_file = device_create_file(&(dev->dev), &dev_attr_battery_oc_protect_level);
#endif

#ifdef BATTERY_PERCENT_PROTECT
	ret_device_file = device_create_file(&(dev->dev), &dev_attr_battery_percent_protect_ut);
	ret_device_file = device_create_file(&(dev->dev), &dev_attr_battery_percent_protect_stop);
	ret_device_file = device_create_file(&(dev->dev), &dev_attr_battery_percent_protect_level);
#endif

#ifdef DLPT_FEATURE_SUPPORT
	ret_device_file = device_create_file(&(dev->dev), &dev_attr_dlpt_ut);
	ret_device_file = device_create_file(&(dev->dev), &dev_attr_dlpt_stop);
	ret_device_file = device_create_file(&(dev->dev), &dev_attr_dlpt_level);
#endif

	ret_device_file = device_create_file(&(dev->dev), &dev_attr_pmic_auxadc_ut);

	PMICLOG("[PMIC] device_create_file for EM : done.\n");

#if PMIC_UT /* UT TBD */
#ifdef LOW_BATTERY_PROTECT
	register_low_battery_notify(&low_bat_test, LOW_BATTERY_PRIO_CPU_B);
	PMICLOG("register_low_battery_notify:done\n");
#endif
#ifdef BATTERY_OC_PROTECT
	register_battery_oc_notify(&bat_oc_test, BATTERY_OC_PRIO_CPU_B);
	PMICLOG("register_battery_oc_notify:done\n");
#endif
#ifdef BATTERY_PERCENT_PROTECT
	register_battery_percent_notify(&bat_per_test, BATTERY_PERCENT_PRIO_CPU_B);
	PMICLOG("register_battery_percent_notify:done\n");
#endif
#endif
	/*pwrkey_sw_workaround_init(); */
	return 0;
}

static int pmic_mt_remove(struct platform_device *dev)
{
	PMICLOG("******** MT pmic driver remove!! ********\n");

	return 0;
}

static void pmic_mt_shutdown(struct platform_device *dev)
{
	PMICLOG("******** MT pmic driver shutdown!! ********\n");
	vmd1_pmic_setting_on();
}

static int pmic_mt_suspend(struct platform_device *dev, pm_message_t state)
{
	pmic_suspend_state = true;

	PMICLOG("******** MT pmic driver suspend!! ********\n");

#ifdef LOW_BATTERY_PROTECT
	lbat_min_en_setting(0);
	lbat_max_en_setting(0);

	PMICLOG("Reg[0x%x]=0x%x, Reg[0x%x]=0x%x, Reg[0x%x]=0x%x\n",
		MT6351_AUXADC_LBAT3, upmu_get_reg_value(MT6351_AUXADC_LBAT3),
		MT6351_AUXADC_LBAT4, upmu_get_reg_value(MT6351_AUXADC_LBAT4),
		MT6351_INT_CON0, upmu_get_reg_value(MT6351_INT_CON0)
	    );
#endif

#ifdef BATTERY_OC_PROTECT
	bat_oc_h_en_setting(0);
	bat_oc_l_en_setting(0);

	PMICLOG("Reg[0x%x]=0x%x, Reg[0x%x]=0x%x, Reg[0x%x]=0x%x\n",
		MT6351_PMIC_FG_CUR_HTH_ADDR, upmu_get_reg_value(MT6351_PMIC_FG_CUR_HTH_ADDR),
		MT6351_PMIC_FG_CUR_LTH_ADDR, upmu_get_reg_value(MT6351_PMIC_FG_CUR_LTH_ADDR),
		MT6351_PMIC_RG_INT_EN_FG_BAT_H_ADDR, upmu_get_reg_value(MT6351_PMIC_RG_INT_EN_FG_BAT_H_ADDR)
	    );
#endif

	return 0;
}

static int pmic_mt_resume(struct platform_device *dev)
{
	pmic_suspend_state = false;

	PMICLOG("******** MT pmic driver resume!! ********\n");

#ifdef LOW_BATTERY_PROTECT
	lbat_min_en_setting(0);
	lbat_max_en_setting(0);
	mdelay(1);

	if (g_low_battery_level == 1) {
		lbat_min_en_setting(1);
		lbat_max_en_setting(1);
	} else if (g_low_battery_level == 2) {
		/*lbat_min_en_setting(0);*/
		lbat_max_en_setting(1);
	} else {		/*0*/
		lbat_min_en_setting(1);
		/*lbat_max_en_setting(0);*/
	}

	PMICLOG("Reg[0x%x]=0x%x, Reg[0x%x]=0x%x, Reg[0x%x]=0x%x\n",
		MT6351_AUXADC_LBAT3, upmu_get_reg_value(MT6351_AUXADC_LBAT3),
		MT6351_AUXADC_LBAT4, upmu_get_reg_value(MT6351_AUXADC_LBAT4),
		MT6351_INT_CON0, upmu_get_reg_value(MT6351_INT_CON0)
	    );
#endif

#ifdef BATTERY_OC_PROTECT
	bat_oc_h_en_setting(0);
	bat_oc_l_en_setting(0);
	mdelay(1);

	if (g_battery_oc_level == 1)
		bat_oc_h_en_setting(1);
	else
		bat_oc_l_en_setting(1);

	PMICLOG("Reg[0x%x]=0x%x, Reg[0x%x]=0x%x, Reg[0x%x]=0x%x\n",
		MT6351_PMIC_FG_CUR_HTH_ADDR, upmu_get_reg_value(MT6351_PMIC_FG_CUR_HTH_ADDR),
		MT6351_PMIC_FG_CUR_LTH_ADDR, upmu_get_reg_value(MT6351_PMIC_FG_CUR_LTH_ADDR),
		MT6351_PMIC_RG_INT_EN_FG_BAT_H_ADDR, upmu_get_reg_value(MT6351_PMIC_RG_INT_EN_FG_BAT_H_ADDR)
	    );
#endif

	return 0;
}

struct platform_device pmic_mt_device = {
	.name = "mt-pmic",
	.id = -1,
};

static struct platform_driver pmic_mt_driver_probe = {
	.probe = pmic_mt_probe,
	.remove = pmic_mt_remove,
	.shutdown = pmic_mt_shutdown,
	/*#ifdef CONFIG_PM*/
	.suspend = pmic_mt_suspend,
	.resume = pmic_mt_resume,
	/*#endif*/
	.driver = {
		   .name = "mt-pmic",
		   },
};

static DEFINE_MUTEX(pmic_efuse_lock_mutex);
unsigned int pmic_Read_Efuse_HPOffset(int i)
{
	unsigned int ret = 0;
	unsigned int reg_val = 0;
	unsigned int efusevalue;

	pr_debug("pmic_Read_Efuse_HPOffset(+)\n");
	mutex_lock(&pmic_efuse_lock_mutex);
	/*1. enable efuse ctrl engine clock */
	pmic_config_interface(MT6351_PMIC_RG_EFUSE_CK_PDN_HWEN_ADDR, 0x00,
	MT6351_PMIC_RG_EFUSE_CK_PDN_HWEN_MASK, MT6351_PMIC_RG_EFUSE_CK_PDN_HWEN_SHIFT);
	pmic_config_interface(MT6351_PMIC_RG_EFUSE_CK_PDN_ADDR, 0x00,
		MT6351_PMIC_RG_EFUSE_CK_PDN_MASK, MT6351_PMIC_RG_EFUSE_CK_PDN_SHIFT);
	/*2. */
	pmic_config_interface(MT6351_PMIC_RG_OTP_RD_SW_ADDR, 0x01,
	MT6351_PMIC_RG_OTP_RD_SW_MASK, MT6351_PMIC_RG_OTP_RD_SW_SHIFT);

	/*3. set row to read */
	pmic_config_interface(MT6351_PMIC_RG_OTP_PA_ADDR, i*2,
	MT6351_PMIC_RG_OTP_PA_MASK, MT6351_PMIC_RG_OTP_PA_SHIFT);
	/*4. Toggle */
	udelay(100);
	ret = pmic_read_interface(MT6351_PMIC_RG_OTP_RD_TRIG_ADDR, &reg_val,
		MT6351_PMIC_RG_OTP_RD_TRIG_MASK, MT6351_PMIC_RG_OTP_RD_TRIG_SHIFT);

	if (reg_val == 0) {
		pmic_config_interface(MT6351_PMIC_RG_OTP_RD_TRIG_ADDR, 1,
			MT6351_PMIC_RG_OTP_RD_TRIG_MASK, MT6351_PMIC_RG_OTP_RD_TRIG_SHIFT);
	} else{
		pmic_config_interface(MT6351_PMIC_RG_OTP_RD_TRIG_ADDR, 0,
			MT6351_PMIC_RG_OTP_RD_TRIG_MASK, MT6351_PMIC_RG_OTP_RD_TRIG_SHIFT);
	}

	/*5. polling Reg[0x61A] */
	udelay(300);
	do {
		ret = pmic_read_interface(MT6351_PMIC_RG_OTP_RD_BUSY_ADDR, &reg_val,
		MT6351_PMIC_RG_OTP_RD_BUSY_MASK, MT6351_PMIC_RG_OTP_RD_BUSY_SHIFT);
	} while (reg_val == 1);
	udelay(1000);		/*Need to delay at least 1ms for 0x61A and than can read 0xC18 */
	/*6. read data */
	ret = pmic_read_interface(MT6351_PMIC_RG_OTP_DOUT_SW_ADDR, &efusevalue,
	MT6351_PMIC_RG_OTP_DOUT_SW_MASK, MT6351_PMIC_RG_OTP_DOUT_SW_SHIFT);

	pr_debug("HPoffset : efuse=0x%x\n", efusevalue);
	/*7. Disable efuse ctrl engine clock */
	pmic_config_interface(MT6351_PMIC_RG_EFUSE_CK_PDN_HWEN_ADDR, 0x01,
		MT6351_PMIC_RG_EFUSE_CK_PDN_HWEN_MASK, MT6351_PMIC_RG_EFUSE_CK_PDN_HWEN_SHIFT);
	pmic_config_interface(MT6351_PMIC_RG_EFUSE_CK_PDN_ADDR, 0x01,
		MT6351_PMIC_RG_EFUSE_CK_PDN_MASK, MT6351_PMIC_RG_EFUSE_CK_PDN_SHIFT);

	mutex_unlock(&pmic_efuse_lock_mutex);
	return efusevalue;
}

/*****************************************************************************
 * PMIC mudule init/exit
 ******************************************************************************/
static int __init pmic_mt_init(void)
{
	int ret;

#if !defined CONFIG_HAS_WAKELOCKS
	wakeup_source_init(&pmicThread_lock, "pmicThread_lock_mt6328 wakelock");
	wakeup_source_init(&bat_percent_notify_lock, "bat_percent_notify_lock wakelock");
#else
	wake_lock_init(&pmicThread_lock, WAKE_LOCK_SUSPEND, "pmicThread_lock_mt6328 wakelock");
	wake_lock_init(&bat_percent_notify_lock, WAKE_LOCK_SUSPEND,
		       "bat_percent_notify_lock wakelock");
#endif

#if !defined CONFIG_HAS_WAKELOCKS
	wakeup_source_init(&dlpt_notify_lock, "dlpt_notify_lock wakelock");
#else
	wake_lock_init(&dlpt_notify_lock, WAKE_LOCK_SUSPEND, "dlpt_notify_lock wakelock");
#endif

#if !defined CONFIG_MTK_LEGACY
#ifdef CONFIG_OF
	PMICLOG("pmic_regulator_init_OF\n");

	/* PMIC device driver register*/
	ret = platform_device_register(&pmic_mt_device);
	if (ret) {
		PMICLOG("****[pmic_mt_init] Unable to device register(%d)\n", ret);
		return ret;
	}
	ret = platform_driver_register(&pmic_mt_driver_probe);
	if (ret) {
		PMICLOG("****[pmic_mt_init] Unable to register driver (%d)\n", ret);
		return ret;
	}
	ret = platform_driver_register(&mt_pmic_driver_probe);
	if (ret) {
		PMICLOG("****[pmic_mt_init] Unable to register driver by DT(%d)\n", ret);
		return ret;
	}
#endif				/* End of #ifdef CONFIG_OF */
#else
	PMICLOG("pmic_regulator_init\n");
	/* PMIC device driver register*/
	ret = platform_device_register(&pmic_mt_device);
	if (ret) {
		PMICLOG("****[pmic_mt_init] Unable to device register(%d)\n", ret);
		return ret;
	}
	ret = platform_driver_register(&pmic_mt_driver_probe);
	if (ret) {
		PMICLOG("****[pmic_mt_init] Unable to register driver (%d)\n", ret);
		return ret;
	}
#endif				/* End of #if !defined CONFIG_MTK_LEGACY */


	pmic_auxadc_init();

	pr_debug("****[pmic_mt_init] Initialization : DONE !!\n");

	return 0;
}

static void __exit pmic_mt_exit(void)
{
#if !defined CONFIG_MTK_LEGACY
#ifdef CONFIG_OF
	platform_driver_unregister(&mt_pmic_driver_probe);
#endif
#endif				/* End of #if !defined CONFIG_MTK_LEGACY */
}
fs_initcall(pmic_mt_init);

/*module_init(pmic_mt_init);*/
module_exit(pmic_mt_exit);

MODULE_AUTHOR("Argus Lin");
MODULE_DESCRIPTION("MT PMIC Device Driver");
MODULE_LICENSE("GPL");

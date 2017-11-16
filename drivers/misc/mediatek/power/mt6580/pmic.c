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
 * Anderson Tsai
 *
 ****************************************************************************/
#include <asm/uaccess.h>

/* customization */
/* #include <cust_battery_meter.h> TBD */
#ifdef CONFIG_MTK_LEGACY
/* #include <cust_eint.h> TBD */
/* #include <mach/cust_eint.h> TBD ??? */
#endif
/* #include <cust_pmic.h> TBD */

#include <generated/autoconf.h>

/* Linux include header */
/* #include <linux/aee.h> TBD */
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
/* #include <linux/earlysuspend.h> TBD */
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/interrupt.h>
/* #include <linux/irqchip/mt-eic.h> TBD ??? */
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_fdt.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/syscalls.h>
#include <linux/time.h>
#include <linux/wakelock.h>
#include <linux/writeback.h>
#include <linux/notifier.h>
#include <linux/fb.h>
/* #include <linux/xlog.h> TBD */

/* mach */
/* #include <mach/battery_common.h> TBD */
/* #include <mach/battery_meter.h> TBD */
#include <mt-plat/battery_meter.h>
#include <mt-plat/battery_common.h>
#include <mach/mt_battery_meter.h>

/* #include <mach/eint.h> TBD */
#if defined(CONFIG_MTK_KERNEL_POWER_OFF_CHARGING)
#include <mt-plat/mt_boot.h>

#endif
/* #include <mach/mt_gpio.h> TBD */
#if defined(CONFIG_MTK_KERNEL_POWER_OFF_CHARGING)
#include <mach/mt_gpt.h>
#endif
#if defined(CONFIG_MTK_PMIC_WRAP)
#include <mach/mt_pmic_wrap.h>
#endif
#include <mach/mt_spm_mtcmos.h>
#include <mt-plat/mtk_rtc.h>
/* #include <mach/pmic.h> TBD */
#if defined(CONFIG_MTK_KERNEL_POWER_OFF_CHARGING)
/*#include <mach/system.h>*/
#endif
/* #include <mach/upmu_common.h> */
#include <mt-plat/upmu_common.h>
#include "pmic.h"
#include "pmic_dvt.h"

/*
 * PMIC extern variable
 */
#if defined(CONFIG_MTK_KERNEL_POWER_OFF_CHARGING_FIX)
static bool long_pwrkey_press;
static unsigned long timer_pre;
static unsigned long timer_pos;
/* 500ms */
#define LONG_PWRKEY_PRESS_TIME		(500 * 1000000)
#endif

/*
 * Global variable
 */
unsigned int g_eint_pmic_num = 17;
unsigned int g_cust_eint_mt_pmic_debounce_cn = 1;
/* #define IRQ_TYPE_LEVEL_HIGH       4 */
unsigned int g_cust_eint_mt_pmic_type = 4;
unsigned int g_cust_eint_mt_pmic_debounce_en = 1;

/*
 * PMIC related define
 */
static DEFINE_MUTEX(pmic_lock_mutex);
#define PMIC_EINT_SERVICE

/*
 * PMIC read/write APIs
 */
#if 0				/* defined(CONFIG_MTK_FPGA) */
    /* no CONFIG_PMIC_HW_ACCESS_EN */
#else
#define CONFIG_PMIC_HW_ACCESS_EN
#endif


static DEFINE_MUTEX(pmic_access_mutex);

unsigned int pmic_read_interface(unsigned int RegNum, unsigned int *val, unsigned int MASK,
					unsigned int SHIFT)
{
	unsigned int return_value = 0;

#if defined(CONFIG_PMIC_HW_ACCESS_EN)
	unsigned int pmic_reg = 0;
	unsigned int rdata = 0xFFFF;

	/* mt_read_byte(RegNum, &pmic_reg); */
#if defined(CONFIG_MTK_PMIC_WRAP)
	return_value = pwrap_wacs2(0, (RegNum), 0, &rdata);
#endif
	pmic_reg = rdata;
	if (return_value != 0) {
		PMICLOG("[pmic_read_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
		return return_value;
	}
	/* PMICLOG"[pmic_read_interface] Reg[%x]=0x%x\n", RegNum, pmic_reg); */

	pmic_reg &= (MASK << SHIFT);
	*val = (pmic_reg >> SHIFT);
	/* PMICLOG"[pmic_read_interface] val=0x%x\n", *val); */

#else
	/* PMICLOG("[pmic_read_interface] Can not access HW PMIC\n"); */
#endif

	return return_value;
}

unsigned int pmic_config_interface(unsigned int RegNum, unsigned int val, unsigned int MASK,
					unsigned int SHIFT)
{
	unsigned int return_value = 0;

#if defined(CONFIG_PMIC_HW_ACCESS_EN)
	unsigned int pmic_reg = 0;
	unsigned int rdata = 0xFFFF;

	mutex_lock(&pmic_access_mutex);

	/* 1. mt_read_byte(RegNum, &pmic_reg); */
#if defined(CONFIG_MTK_PMIC_WRAP)
	return_value = pwrap_wacs2(0, (RegNum), 0, &rdata);
#endif
	pmic_reg = rdata;
	if (return_value != 0) {
		PMICLOG("[pmic_config_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
		mutex_unlock(&pmic_access_mutex);
		return return_value;
	}
	/* PMICLOG"[pmic_config_interface] Reg[%x]=0x%x\n", RegNum, pmic_reg); */

	pmic_reg &= ~(MASK << SHIFT);
	pmic_reg |= (val << SHIFT);

	/* 2. mt_write_byte(RegNum, pmic_reg); */
#if defined(CONFIG_MTK_PMIC_WRAP)
	return_value = pwrap_wacs2(1, (RegNum), pmic_reg, &rdata);
#endif
	if (return_value != 0) {
		PMICLOG("[pmic_config_interface] Reg[%x]= pmic_wrap write data fail\n", RegNum);
		mutex_unlock(&pmic_access_mutex);
		return return_value;
	}
	/* PMICLOG"[pmic_config_interface] write Reg[%x]=0x%x\n", RegNum, pmic_reg); */

#if 0
	/* 3. Double Check */
	/* mt_read_byte(RegNum, &pmic_reg); */
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
	/* PMICLOG("[pmic_config_interface] Can not access HW PMIC\n"); */
#endif

	return return_value;
}

unsigned int pmic_read_interface_nolock(unsigned int RegNum, unsigned int *val, unsigned int MASK,
					unsigned int SHIFT)
{
	return pmic_read_interface(RegNum, val, MASK, SHIFT);
}

unsigned int pmic_config_interface_nolock(unsigned int RegNum, unsigned int val, unsigned int MASK,
					  unsigned int SHIFT)
{
	unsigned int return_value = 0;

#if defined(CONFIG_PMIC_HW_ACCESS_EN)
	unsigned int pmic_reg = 0;
	unsigned int rdata = 0xFFFF;

	/* pmic wrapper has spinlock protection. pmic do not to do it again */

	/*1. mt_read_byte(RegNum, &pmic_reg); */
#if defined(CONFIG_MTK_PMIC_WRAP)
	return_value = pwrap_wacs2(0, (RegNum), 0, &rdata);
#endif
	pmic_reg = rdata;
	if (return_value != 0) {
		PMICLOG("[pmic_config_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
		return return_value;
	}
	/*PMICLOG"[pmic_config_interface] Reg[%x]=0x%x\n", RegNum, pmic_reg); */

	pmic_reg &= ~(MASK << SHIFT);
	pmic_reg |= (val << SHIFT);

	/*2. mt_write_byte(RegNum, pmic_reg); */
#if defined(CONFIG_MTK_PMIC_WRAP)
	return_value = pwrap_wacs2(1, (RegNum), pmic_reg, &rdata);
#endif
	if (return_value != 0) {
		PMICLOG("[pmic_config_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
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

/*
 * PMIC lock/unlock APIs
 */
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

/**********************************************************
  *
  *   [Internal Function]
  *
  *********************************************************/
void mt6350_dump_register(void)
{
	unsigned char i = 0;

	PMICLOG("dump PMIC 6350 register\n");

	for (i = 0; i <= 0x079a; i = i + 10) {
		PMICLOG
		    ("Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x\n",
		     i, upmu_get_reg_value(i), i + 1, upmu_get_reg_value(i + 1), i + 2,
		     upmu_get_reg_value(i + 2), i + 3, upmu_get_reg_value(i + 3), i + 4,
		     upmu_get_reg_value(i + 4));

		PMICLOG
		    ("Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x\n",
		     i + 5, upmu_get_reg_value(i + 5), i + 6, upmu_get_reg_value(i + 6), i + 7,
		     upmu_get_reg_value(i + 7), i + 8, upmu_get_reg_value(i + 8), i + 9,
		     upmu_get_reg_value(i + 9));
	}

}

/*
 * upmu_interrupt_chrdet_int_en
 */
void upmu_interrupt_chrdet_int_en(unsigned int val)
{
	PMICLOG_DBG("[upmu_interrupt_chrdet_int_en] val=%d.\r\n", val);

	pmic_set_register_value(PMIC_RG_INT_EN_CHRDET, val);
}
EXPORT_SYMBOL(upmu_interrupt_chrdet_int_en);

/*
 * PMIC charger detection
 */
unsigned int upmu_get_rgs_chrdet(void)
{
	unsigned int val = 0;

	val = pmic_get_register_value(PMIC_RGS_CHRDET);
	PMICLOG_DBG("[upmu_get_rgs_chrdet] CHRDET status = %d\n", val);

	return val;
}

/*
 * mt-pmic dev_attr APIs
 */
unsigned int g_reg_value = 0;
static ssize_t show_pmic_access(struct device *dev, struct device_attribute *attr, char *buf)
{
	PMICLOG("[show_pmic_access] 0x%x\n", g_reg_value);
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

static DEVICE_ATTR(pmic_access, 0664, show_pmic_access, store_pmic_access);	/* 664 */

/*
 * DVT entry
 */
unsigned char g_reg_value_pmic = 0;

static ssize_t show_pmic_dvt(struct device *dev, struct device_attribute *attr, char *buf)
{
	PMICLOG("[show_pmic_dvt] 0x%x\n", g_reg_value_pmic);
	return sprintf(buf, "%u\n", g_reg_value_pmic);
}

static ssize_t store_pmic_dvt(struct device *dev, struct device_attribute *attr, const char *buf,
			      size_t size)
{
	int ret = 0;
	unsigned int test_item = 0;

	PMICLOG("[store_pmic_dvt]\n");

	if (buf != NULL && size != 0) {
		PMICLOG("[store_pmic_dvt] buf is %s and size is %zu\n", buf, size);

		/*test_item = simple_strtoul(buf, &pvalue, 10); */
		ret = kstrtoul(buf, 10, (unsigned long *)&test_item);
		PMICLOG("[store_pmic_dvt] test_item=%d\n", test_item);

#ifdef MTK_PMIC_DVT_SUPPORT
		pmic_dvt_entry(test_item);
#else
		PMICLOG("[store_pmic_dvt] no define MTK_PMIC_DVT_SUPPORT\n");
#endif
	}
	return size;
}

static DEVICE_ATTR(pmic_dvt, 0664, show_pmic_dvt, store_pmic_dvt);


/*
 * PMIC6350 linux reguplator driver
 */
/*extern PMU_FLAG_TABLE_ENTRY pmu_flags_table[];*/

#define regulator_log 0

static int mtk_regulator_enable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	unsigned int add = 0, val = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);
	if (mreg->en_reg != 0) {
		pmic_set_register_value(mreg->en_reg, 1);
		add = pmu_flags_table[mreg->en_reg].offset;
		val = upmu_get_reg_value(pmu_flags_table[mreg->en_reg].offset);
	} else {
		pr_err(PMICTAG "regulator_enable fail, no en_reg(name=%s)\n", rdesc->name);
		return -1;
	}
#if regulator_log
	PMICLOG("regulator_enable(name=%s id=%d en_reg=%x vol_reg=%x) [%x]=0x%x\n", rdesc->name,
		rdesc->id, mreg->en_reg, mreg->vol_reg, add, val);
#endif
	return 0;
}

static int mtk_regulator_disable(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	unsigned int add = 0, val = 0;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	if (rdev->use_count == 0) {
		pr_err(PMICTAG "regulator_disable fail (name=%s use_count=%d)\n", rdesc->name,
			rdev->use_count);
		return -1;
	}

	if (mreg->en_reg != 0) {
		pmic_set_register_value(mreg->en_reg, 0);
		add = pmu_flags_table[mreg->en_reg].offset;
		val = upmu_get_reg_value(pmu_flags_table[mreg->en_reg].offset);
	} else {
		pr_err(PMICTAG "regulator_disable fail, no en_reg(name=%s)\n", rdesc->name);
		return -1;
	}

#if regulator_log
	PMICLOG("regulator_disable(name=%s id=%d en_reg=%x vol_reg=%x use_count=%d) [%x]=0x%x\n",
		rdesc->name, rdesc->id, mreg->en_reg, mreg->vol_reg, rdev->use_count, add, val);
#endif
	return 0;
}

static int mtk_regulator_is_enabled(struct regulator_dev *rdev)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;
	int en;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

	en = pmic_get_register_value(mreg->en_reg);

#if regulator_log
	PMICLOG("[PMIC]regulator_is_enabled(name=%s id=%d en_reg=%x vol_reg=%x en=%d)\n",
		rdesc->name, rdesc->id, mreg->en_reg, mreg->vol_reg, en);
#endif
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
				voltage = pVoltage[regVal];
				add = pmu_flags_table[mreg->en_reg].offset;
				val = upmu_get_reg_value(pmu_flags_table[mreg->en_reg].offset);
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

#if regulator_log
	PMICLOG
	    ("regulator_get_voltage_sel(name=%s id=%d en_reg=%x vol_reg=%x reg/sel:%d voltage:%d [0x%x]=0x%x)\n",
	     rdesc->name, rdesc->id, mreg->en_reg, mreg->vol_reg, regVal, voltage, add, val);
#endif
	return regVal;
}

static int mtk_regulator_set_voltage_sel(struct regulator_dev *rdev, unsigned selector)
{
	const struct regulator_desc *rdesc = rdev->desc;
	struct mtk_regulator *mreg;

	mreg = container_of(rdesc, struct mtk_regulator, desc);

#if regulator_log
	PMICLOG("regulator_set_voltage_sel(name=%s id=%d en_reg=%x vol_reg=%x selector=%d)\n",
		rdesc->name, rdesc->id, mreg->en_reg, mreg->vol_reg, selector);
#endif
/* VGP2
    0:1200000,->0
    1:1300000,->1
    2:1500000,->2
    3:1800000,->3
    4:2000000,->7
    5:2500000,->4
    6:2800000,->5
    7:3000000,->6
*/
	if (strcmp(rdesc->name, "VGP2") == 0) {
		if (mreg->vol_reg != 0) {
			if (selector <= 3)
				pmic_set_register_value(mreg->vol_reg, selector);
			else if (selector == 4)
				pmic_set_register_value(mreg->vol_reg, selector + 3);
			else
				pmic_set_register_value(mreg->vol_reg, selector - 1);
		}
	} else {
		if (mreg->vol_reg != 0)
			pmic_set_register_value(mreg->vol_reg, selector);
	}

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

#if regulator_log
	PMICLOG("regulator_list_voltage(name=%s id=%d en_reg=%x vol_reg=%x selector=%d voltage=%d)\n", rdesc->name,
		desc->id, mreg->en_reg, mreg->vol_reg, selector, voltage);
#endif
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

	PMICLOG("[EM] LDO_%s_STATUS : %d\n", mreg->desc.name, ret_value);
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
	PMICLOG("[EM] LDO_%s_VOLTAGE : %d\n", mreg->desc.name, ret_value);
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

	PMICLOG("[EM] BUCK_%s_STATUS : %d\n", mreg->desc.name, ret_value);
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
			PMICLOG("[EM][ERROR] buck_%s_VOLTAGE : voltage=0 vol_reg=0\n",
				mreg->desc.name);
		}
	} else {
		pVoltage = (const int *)mreg->pvoltages;
		ret_value = pVoltage[0];
	}

	ret_value = ret_value / 1000;
	PMICLOG("[EM] BUCK_%s_VOLTAGE : %d\n", mreg->desc.name, ret_value);
	return sprintf(buf, "%u\n", ret_value);
}

static ssize_t store_BUCK_VOLTAGE(struct device *dev, struct device_attribute *attr,
				  const char *buf, size_t size)
{
	PMICLOG("[EM] Not Support Write Function\n");
	return size;
}

/* LDO Voltage Table */
static const int pmic_1v8_voltages[] = {
	1800000,
};

static const int pmic_2v2_voltages[] = {
	2200000,
};

static const int pmic_2v8_voltages[] = {
	2800000,
};

static const int pmic_3v3_voltages[] = {
	3300000,
};

static const int VA_voltages[] = {
	1800000,
	2500000,
};

static const int VCAMA_voltages[] = {
	1500000,
	1800000,
	2500000,
	2800000,
};

static const int VCN33_voltages[] = {
	3300000,
	3400000,
	3500000,
	3600000,
};

static const int VCN28_voltages[] = {
	1800000,
	2800000,
};

static const int VCAMD_voltages[] = {
	1200000,
	1300000,
	1500000,
	1800000,
};

static const int VMC_voltages[] = {
	1800000,
	3300000,
};

static const int VMCH_voltages[] = {
	3000000,
	3300000,
};

static const int VEMC3V3_voltages[] = {
	3000000,
	3300000,
};

static const int VGP1_voltages[] = {
	1200000,
	1300000,
	1500000,
	1800000,
	2000000,
	2800000,
	3000000,
	3300000,
};

static const int VGP2_voltages[] = {
	1200000,
	1300000,
	1500000,
	1800000,
	2000000,
	2500000,
	2800000,
	3000000,
};

static const int VGP3_voltages[] = {
	1200000,
	1300000,
	1500000,
	1800000,
};

static const int VCAMAF_voltages[] = {
	1200000,
	1300000,
	1500000,
	1800000,
	2000000,
	2800000,
	3000000,
	3300000,
};

static const int VSIM1_voltages[] = {
	1800000,
	3000000,
};

static const int VSIM2_voltages[] = {
	1800000,
	3000000,
};

static const int VIBR_voltages[] = {
	1200000,
	1300000,
	1500000,
	1800000,
	2000000,
	2800000,
	3000000,
	3300000,
};

static const int VM_voltages[] = {
	1200000,
	1350000,
	1500000,
	1800000,
};

/*
ALDO        VCN28
ALDO        VTCXO
ALDO        VA
ALDO        VCAMA
DLDO        VCN_3V3
DLDO        VIO28
DLDO        VSIM1
DLDO        VSIM2
DLDO        VUSB
DLDO        VGP1
DLDO        VGP2
DLDO        VEMC_3V3
DLDO        VCAM_AF
DLDO        VMC
DLDO        VMCH
DLDO        VIBR
RTCLDO      VRTC
VSYS LDO    VM
VSYS LDO    VRF18
VSYS LDO    VIO18
VSYS LDO    VCAMD
VSYS LDO    VCAM_IO
VSYS LDO    VGP3
VSYS LDO    VCN_1V8
*/

#if defined(CONFIG_MTK_LEGACY)
struct mtk_regulator mtk_ldos[] = {
	PMIC_LDO_GEN1(VCN28, PMIC_RG_VCN28_EN, NULL, pmic_2v8_voltages, 1, PMIC_EN),
	PMIC_LDO_GEN1(VTCXO, PMIC_RG_VTCXO_EN, NULL, pmic_2v2_voltages, 0, PMIC_EN),
	PMIC_LDO_GEN1(VA, PMIC_RG_VA_EN, PMIC_RG_VA_VOSEL, VA_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(VCAMA, PMIC_RG_VCAMA_EN, PMIC_RG_VCAMA_VOSEL, VCAMA_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(VCN33_BT, PMIC_RG_VCN33_EN_BT, PMIC_RG_VCN33_VOSEL, VCN33_voltages, 1,
		      PMIC_EN_VOL),
	PMIC_LDO_GEN1(VCN33_WIFI, PMIC_RG_VCN33_EN_WIFI, PMIC_RG_VCN33_VOSEL, VCN33_voltages, 1,
		      PMIC_EN_VOL),
	PMIC_LDO_GEN1(VIO28, PMIC_VIO28_EN, NULL, pmic_2v8_voltages, 0, PMIC_EN),
	PMIC_LDO_GEN1(VSIM1, PMIC_RG_VSIM1_EN, PMIC_RG_VSIM1_VOSEL, VSIM1_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(VSIM2, PMIC_RG_VSIM2_EN, PMIC_RG_VSIM2_VOSEL, VSIM2_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(VUSB, PMIC_RG_VUSB_EN, NULL, pmic_3v3_voltages, 1, PMIC_EN),
	PMIC_LDO_GEN1(VGP1, PMIC_RG_VGP1_EN, PMIC_RG_VGP1_VOSEL, VGP1_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(VGP2, PMIC_RG_VGP2_EN, PMIC_RG_VGP2_VOSEL, VGP2_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(VEMC_3V3, PMIC_RG_VEMC_3V3_EN, PMIC_RG_VEMC_3V3_VOSEL, VEMC3V3_voltages, 1,
		      PMIC_EN_VOL),
	PMIC_LDO_GEN1(VCAMAF, PMIC_RG_VCAM_AF_EN, PMIC_RG_VCAM_AF_VOSEL, VCAMAF_voltages, 1,
		      PMIC_EN_VOL),
	PMIC_LDO_GEN1(VMC, PMIC_RG_VMC_EN, PMIC_RG_VMC_VOSEL, VMC_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(VMCH, PMIC_RG_VMCH_EN, PMIC_RG_VMCH_VOSEL, VMCH_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(VIBR, PMIC_RG_VIBR_EN, PMIC_RG_VIBR_VOSEL, VIBR_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(VRTC, PMIC_VRTC_EN, NULL, pmic_2v8_voltages, 0, PMIC_EN),
	PMIC_LDO_GEN1(VM, PMIC_RG_VM_EN, PMIC_RG_VM_VOSEL, VM_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(VRF18, PMIC_RG_VRF18_EN, NULL, pmic_1v8_voltages, 1, PMIC_EN),
	PMIC_LDO_GEN1(VIO18, PMIC_RG_VIO18_EN, NULL, pmic_1v8_voltages, 1, PMIC_EN),
	PMIC_LDO_GEN1(VCAMD, PMIC_RG_VCAMD_EN, PMIC_RG_VCAMD_VOSEL, VCAMD_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(VCAMIO, PMIC_RG_VCAM_IO_EN, NULL, pmic_1v8_voltages, 1, PMIC_EN),
	PMIC_LDO_GEN1(VGP3, PMIC_RG_VGP3_EN, PMIC_RG_VGP3_VOSEL, VGP3_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(VCN_1V8, PMIC_RG_VCN_1V8_EN, NULL, pmic_1v8_voltages, 1, PMIC_EN),
};


/* Buck only provide query voltage & enable */
static struct mtk_regulator mtk_bucks[] = {
	PMIC_BUCK_GEN(VPA, PMIC_QI_VPA_EN, PMIC_NI_VPA_VOSEL, 500000, 3650000, 50000),
	PMIC_BUCK_GEN(VPROC, PMIC_QI_VPROC_EN, PMIC_NI_VPROC_VOSEL, 700000, 1493750, 6250),
};

#else

struct mtk_regulator mtk_ldos[] = {
	PMIC_LDO_GEN1(vcn28, PMIC_RG_VCN28_EN, NULL, pmic_2v8_voltages, 1, PMIC_EN),
	PMIC_LDO_GEN1(vtcxo, PMIC_RG_VTCXO_EN, NULL, pmic_2v2_voltages, 0, PMIC_EN),
	PMIC_LDO_GEN1(va, PMIC_RG_VA_EN, PMIC_RG_VA_VOSEL, VA_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vcama, PMIC_RG_VCAMA_EN, PMIC_RG_VCAMA_VOSEL, VCAMA_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vcn33_bt, PMIC_RG_VCN33_EN_BT, PMIC_RG_VCN33_VOSEL, VCN33_voltages, 1,
		      PMIC_EN_VOL),
	PMIC_LDO_GEN1(vcn33_wifi, PMIC_RG_VCN33_EN_WIFI, PMIC_RG_VCN33_VOSEL, VCN33_voltages, 1,
		      PMIC_EN_VOL),
	PMIC_LDO_GEN1(vio28, PMIC_VIO28_EN, NULL, pmic_2v8_voltages, 0, PMIC_EN),
	PMIC_LDO_GEN1(vsim1, PMIC_RG_VSIM1_EN, PMIC_RG_VSIM1_VOSEL, VSIM1_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vsim2, PMIC_RG_VSIM2_EN, PMIC_RG_VSIM2_VOSEL, VSIM2_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vusb, PMIC_RG_VUSB_EN, NULL, pmic_3v3_voltages, 1, PMIC_EN),
	PMIC_LDO_GEN1(vgp1, PMIC_RG_VGP1_EN, PMIC_RG_VGP1_VOSEL, VGP1_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vgp2, PMIC_RG_VGP2_EN, PMIC_RG_VGP2_VOSEL, VGP2_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vemc_3v3, PMIC_RG_VEMC_3V3_EN, PMIC_RG_VEMC_3V3_VOSEL, VEMC3V3_voltages, 1,
		      PMIC_EN_VOL),
	PMIC_LDO_GEN1(vcamaf, PMIC_RG_VCAM_AF_EN, PMIC_RG_VCAM_AF_VOSEL, VCAMAF_voltages, 1,
		      PMIC_EN_VOL),
	PMIC_LDO_GEN1(vmc, PMIC_RG_VMC_EN, PMIC_RG_VMC_VOSEL, VMC_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vmch, PMIC_RG_VMCH_EN, PMIC_RG_VMCH_VOSEL, VMCH_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vibr, PMIC_RG_VIBR_EN, PMIC_RG_VIBR_VOSEL, VIBR_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vrtc, PMIC_VRTC_EN, NULL, pmic_2v8_voltages, 0, PMIC_EN),
	PMIC_LDO_GEN1(vm, PMIC_RG_VM_EN, PMIC_RG_VM_VOSEL, VM_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vrf18, PMIC_RG_VRF18_EN, NULL, pmic_1v8_voltages, 1, PMIC_EN),
	PMIC_LDO_GEN1(vio18, PMIC_RG_VIO18_EN, NULL, pmic_1v8_voltages, 1, PMIC_EN),
	PMIC_LDO_GEN1(vcamd, PMIC_RG_VCAMD_EN, PMIC_RG_VCAMD_VOSEL, VCAMD_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vcamio, PMIC_RG_VCAM_IO_EN, NULL, pmic_1v8_voltages, 1, PMIC_EN),
	PMIC_LDO_GEN1(vgp3, PMIC_RG_VGP3_EN, PMIC_RG_VGP3_VOSEL, VGP3_voltages, 1, PMIC_EN_VOL),
	PMIC_LDO_GEN1(vcn_1v8, PMIC_RG_VCN_1V8_EN, NULL, pmic_1v8_voltages, 1, PMIC_EN),
};

/* Buck only provide query voltage & enable */
static struct mtk_regulator mtk_bucks[] = {
	PMIC_BUCK_GEN(vpa, PMIC_QI_VPA_EN, PMIC_NI_VPA_VOSEL, 500000, 3650000, 50000),
	PMIC_BUCK_GEN(vproc, PMIC_QI_VPROC_EN, PMIC_NI_VPROC_VOSEL, 700000, 1493750, 6250),
};

#endif				/* End of #if defined(CONFIG_MTK_LEGACY) */
#if !defined CONFIG_MTK_LEGACY
/*
ALDO        VCN28
ALDO        VTCXO
ALDO        VA
ALDO        VCAMA
DLDO        VCN_3V3
DLDO        VIO28
DLDO        VSIM1
DLDO        VSIM2
DLDO        VUSB
DLDO        VGP1
DLDO        VGP2
DLDO        VEMC_3V3
DLDO        VCAM_AF
DLDO        VMC
DLDO        VMCH
DLDO        VIBR
RTCLDO      VRTC
VSYS LDO    VM
VSYS LDO    VRF18
VSYS LDO    VIO18
VSYS LDO    VCAMD
VSYS LDO    VCAM_IO
VSYS LDO    VGP3
VSYS LDO    VCN_1V8
*/

typedef enum MT65XX_POWER_TAG {
	MT6350_POWER_LDO_vcn28 = 0,
	MT6350_POWER_LDO_vtcxo,
	MT6350_POWER_LDO_va,
	MT6350_POWER_LDO_vcama,
	MT6350_POWER_LDO_vcn33_bt,
	MT6350_POWER_LDO_vcn33_wifi,
	MT6350_POWER_LDO_vio28,
	MT6350_POWER_LDO_vsim1,
	MT6350_POWER_LDO_vsim2,
	MT6350_POWER_LDO_vusb,
	MT6350_POWER_LDO_vgp1,
	MT6350_POWER_LDO_vgp2,
	MT6350_POWER_LDO_vemc_3v3,
	MT6350_POWER_LDO_vcamaf,
	MT6350_POWER_LDO_vmc,
	MT6350_POWER_LDO_vmch,
	MT6350_POWER_LDO_vibr,
	MT6350_POWER_LDO_vrtc,
	MT6350_POWER_LDO_vm,
	MT6350_POWER_LDO_vrf18,
	MT6350_POWER_LDO_vio18,
	MT6350_POWER_LDO_vcamd,
	MT6350_POWER_LDO_vcamio,
	MT6350_POWER_LDO_vgp3,
	MT6350_POWER_LDO_vcn_1v8,
	MT6350_POWER_COUNT_END
} MT6350_POWER;

#ifdef CONFIG_OF
/*
#define PMIC_REGULATOR_OF_MATCH(_name, _id)			\
[MT6350_POWER_LDO_##_id] = {						\
	.name = #_name,						\
	.driver_data = &mtk_ldos[MT6350_POWER_LDO_##_id],	\
}
*/
#define PMIC_REGULATOR_OF_MATCH(_name, _id)			\
	{						\
		.name = #_name,						\
		.driver_data = &mtk_ldos[MT6350_POWER_LDO_##_id],	\
	}

static struct of_regulator_match mt6350_regulator_matches[] = {
	PMIC_REGULATOR_OF_MATCH(ldo_vcn28, vcn28),
	PMIC_REGULATOR_OF_MATCH(ldo_vtcxo, vtcxo),
	PMIC_REGULATOR_OF_MATCH(ldo_va, va),
	PMIC_REGULATOR_OF_MATCH(ldo_vcama, vcama),
	PMIC_REGULATOR_OF_MATCH(ldo_vcn33_bt, vcn33_bt),
	PMIC_REGULATOR_OF_MATCH(ldo_vcn33_wifi, vcn33_wifi),
	PMIC_REGULATOR_OF_MATCH(ldo_vio28, vio28),
	PMIC_REGULATOR_OF_MATCH(ldo_vsim1, vsim1),
	PMIC_REGULATOR_OF_MATCH(ldo_vsim2, vsim2),
	PMIC_REGULATOR_OF_MATCH(ldo_vusb, vusb),
	PMIC_REGULATOR_OF_MATCH(ldo_vgp1, vgp1),
	PMIC_REGULATOR_OF_MATCH(ldo_vgp2, vgp2),
	PMIC_REGULATOR_OF_MATCH(ldo_vemc_3v3, vemc_3v3),
	PMIC_REGULATOR_OF_MATCH(ldo_vcamaf, vcamaf),
	PMIC_REGULATOR_OF_MATCH(ldo_vmc, vmc),
	PMIC_REGULATOR_OF_MATCH(ldo_vmch, vmch),
	PMIC_REGULATOR_OF_MATCH(ldo_vibr, vibr),
	PMIC_REGULATOR_OF_MATCH(ldo_vrtc, vrtc),
	PMIC_REGULATOR_OF_MATCH(ldo_vm, vm),
	PMIC_REGULATOR_OF_MATCH(ldo_vrf18, vrf18),
	PMIC_REGULATOR_OF_MATCH(ldo_vio18, vio18),
	PMIC_REGULATOR_OF_MATCH(ldo_vcamd, vcamd),
	PMIC_REGULATOR_OF_MATCH(ldo_vcamio, vcamio),
	PMIC_REGULATOR_OF_MATCH(ldo_vgp3, vgp3),
	PMIC_REGULATOR_OF_MATCH(ldo_vcn_1v8, vcn_1v8),
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
	{.compatible = "mediatek,mt6350",},
	{},
};

MODULE_DEVICE_TABLE(of, pmic_cust_of_ids);
/* TBD START */
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
				     mt6350_regulator_matches,
				     ARRAY_SIZE(mt6350_regulator_matches));
	if ((matched < 0) || (matched != MT6350_POWER_COUNT_END)) {
		PMICLOG("[PMIC]Error parsing regulator init data: %d %d\n", matched,
			MT65XX_POWER_COUNT_END);
		return matched;
	}

	for (i = 0; i < ARRAY_SIZE(mt6350_regulator_matches); i++) {
		if (mtk_ldos[i].isUsedable == 1) {
			mtk_ldos[i].config.dev = &(pdev->dev);
			mtk_ldos[i].config.init_data = mt6350_regulator_matches[i].init_data;
			mtk_ldos[i].config.of_node = mt6350_regulator_matches[i].of_node;
			mtk_ldos[i].config.driver_data = mt6350_regulator_matches[i].driver_data;
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
	const struct of_device_id *match;
	int i;
	unsigned int default_on;

	PMICLOG("[PMIC]pmic_mt_cust_probe %s %s\n", pdev->name, pdev->id_entry->name);

	/* check if device_id is matched */
	match = of_match_device(pmic_cust_of_ids, &pdev->dev);
	if (!match) {
		PMICLOG("[PMIC]pmic_cust_of_ids do not matched\n");
		return -EINVAL;
	}

	/* check customer setting */
	np = of_find_compatible_node(NULL, NULL, "mediatek,mt6350");
	if (np == NULL) {
		pr_info("[PMIC]pmic_mt_cust_probe get node failed\n");
		return -ENOMEM;
	}

	nproot = of_node_get(np);
	if (!nproot) {
		pr_info("[PMIC]pmic_mt_cust_probe of_node_get fail\n");
		return -ENODEV;
	}
	regulators = of_find_node_by_name(nproot, "regulators");
	if (!regulators) {
		pr_info("[PMIC]failed to find regulators node\n");
		return -ENODEV;
	}
	for_each_child_of_node(regulators, child) {
		/* check ldo regualtors and set it */
		for (i = 0; i < ARRAY_SIZE(mt6350_regulator_matches); i++) {
			/* compare dt name & ldos name */
			if (!of_node_cmp(child->name, mt6350_regulator_matches[i].name)) {
				PMICLOG("[PMIC]%s regulator_matches %s\n", child->name,
					(char *)of_get_property(child, "regulator-name", NULL));
				break;
			}
		}
		if (i == ARRAY_SIZE(mt6350_regulator_matches))
			continue;
		if (!of_property_read_u32(child, "regulator-default-on", &default_on)) {
			switch (default_on) {
			case 0:
				/* skip */
				PMICLOG("[PMIC]%s regulator_skip %s\n", child->name,
					mt6350_regulator_matches[i].name);
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
}

static int pmic_mt_cust_remove(struct platform_device *pdev)
{
	/*platform_driver_unregister(&mt_pmic_driver); */
	return 0;
}

static struct platform_driver mt_pmic_driver = {
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
/* TBD END */
void mtk_regulator_init(struct platform_device *dev)
{
#if defined CONFIG_MTK_LEGACY
	int i = 0;
	int ret = 0;
	int isEn = 0;
#endif

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
				/* ret=regulator_enable(mtk_ldos[i].reg); */
			}
		}
	}
#endif				/* End of #if !defined CONFIG_MTK_LEGACY */
}



void pmic6350_regulator_test(void)
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
				PMICLOG("[enable test fail] ret=%d enable=%d addr:0x%x reg:0x%x\n",
					ret1, ret2, pmu_flags_table[mtk_ldos[i].en_reg].offset,
					pmic_get_register_value(mtk_ldos[i].en_reg));
			}

			ret1 = regulator_disable(reg);
			ret2 = regulator_is_enabled(reg);

			if (ret2 == pmic_get_register_value(mtk_ldos[i].en_reg)) {
				PMICLOG("[disable test pass]\n");
			} else {
				PMICLOG("[disable test fail] ret=%d enable=%d addr:0x%x reg:0x%x\n",
					ret1, ret2, pmu_flags_table[mtk_ldos[i].en_reg].offset,
					pmic_get_register_value(mtk_ldos[i].en_reg));
			}

		}
	}

	for (i = 0; i < ARRAY_SIZE(mtk_ldos); i++) {
		const int *pVoltage;

		reg = mtk_ldos[i].reg;
		if (mtk_ldos[i].isUsedable == 1) {
			PMICLOG("[regulator voltage test] %s voltage:%d\n", mtk_ldos[i].desc.name,
				mtk_ldos[i].desc.n_voltages);

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
						     pmic_get_register_value(mtk_ldos[i].vol_reg),
						     pVoltage[j], rvoltage);

					} else {
						PMICLOG
						    ("[%d:%d]:fail  set_voltage:%d  rvoltage:%d\n",
						     j,
						     pmic_get_register_value(mtk_ldos[i].vol_reg),
						     pVoltage[j], rvoltage);
					}
				}
			}
		}
	}


}


/*
 * Low battery call back function
 */
#define LBCB_NUM 16

#define DISABLE_LOW_BATTERY_PROTECT

#ifndef DISABLE_LOW_BATTERY_PROTECT
#define LOW_BATTERY_PROTECT
#endif


#ifdef LOW_BATTERY_PROTECT
/* ex. 3400/5400*4096 */
#define BAT_HV_THD   (POWER_INT0_VOLT*4096/5400)	/* ex: 3400mV */
#define BAT_LV_1_THD (POWER_INT1_VOLT*4096/5400)	/* ex: 3250mV */
#define BAT_LV_2_THD (POWER_INT2_VOLT*4096/5400)	/* ex: 3000mV */

int g_low_battery_level = 0;
int g_low_battery_stop = 0;

struct low_battery_callback_table {
	void *lbcb;
};

struct low_battery_callback_table lbcb_tb[] = {
	{NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL},
	{NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}
};

void (*low_battery_callback)(LOW_BATTERY_LEVEL);

void register_low_battery_notify(void (*low_battery_callback) (LOW_BATTERY_LEVEL),
				 LOW_BATTERY_PRIO prio_val)
{
	PMICLOG("[register_low_battery_notify] start\n");

	lbcb_tb[prio_val].lbcb = low_battery_callback;

	PMICLOG("[register_low_battery_notify] prio_val=%d\n", prio_val);
}

void exec_low_battery_callback(LOW_BATTERY_LEVEL low_battery_level)
{				/* 0:no limit */
	int i = 0;

	if (g_low_battery_stop == 1) {
		PMICLOG("[exec_low_battery_callback] g_low_battery_stop=%d\n", g_low_battery_stop);
	} else {
		for (i = 0; i < LBCB_NUM; i++) {
			if (lbcb_tb[i].lbcb != NULL) {
				low_battery_callback = lbcb_tb[i].lbcb;
				low_battery_callback(low_battery_level);
				PMICLOG("[exec_low_battery_callback] prio_val=%d,low_battery=%d\n",
					i, low_battery_level);
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
	/* default setting */
	pmic_set_register_value(PMIC_AUXADC_LBAT_DEBT_MIN, 0);
	pmic_set_register_value(PMIC_AUXADC_LBAT_DEBT_MAX, 0);
	pmic_set_register_value(PMIC_AUXADC_LBAT_DET_PRD_15_0, 1);
	pmic_set_register_value(PMIC_AUXADC_LBAT_DET_PRD_19_16, 0);

	pmic_set_register_value(PMIC_AUXADC_LBAT_VOLT_MAX, BAT_HV_THD);
	pmic_set_register_value(PMIC_AUXADC_LBAT_VOLT_MIN, BAT_LV_1_THD);

	lbat_min_en_setting(1);
	lbat_max_en_setting(0);

	PMICLOG("Reg[0x%x]=0x%x, Reg[0x%x]=0x%x, Reg[0x%x]=0x%x\n",
		MT6350_AUXADC_LBAT3, upmu_get_reg_value(MT6350_AUXADC_LBAT3),
		MT6350_AUXADC_LBAT4, upmu_get_reg_value(MT6350_AUXADC_LBAT4),
		MT6350_INT_CON0, upmu_get_reg_value(MT6350_INT_CON0)
	    );

	PMICLOG("[low_battery_protect_init] %d mV, %d mV, %d mV\n",
		POWER_INT0_VOLT, POWER_INT1_VOLT, POWER_INT2_VOLT);
	PMICLOG("[low_battery_protect_init] Done\n");

}

#endif				/* #ifdef LOW_BATTERY_PROTECT */

void bat_h_int_handler(void)
{
	PMICLOG_DBG("[bat_h_int_handler]....\n");

	/* sub-task */
#ifdef LOW_BATTERY_PROTECT
	g_low_battery_level = 0;
	exec_low_battery_callback(LOW_BATTERY_LEVEL_0);

#if 0
	lbat_max_en_setting(0);
	mdelay(1);
	lbat_min_en_setting(1);
#else
	pmic_set_register_value(PMIC_AUXADC_LBAT_VOLT_MIN, BAT_LV_1_THD);

	lbat_min_en_setting(0);
	lbat_max_en_setting(0);
	mdelay(1);
	lbat_min_en_setting(1);
#endif

	PMICLOG("Reg[0x%x]=0x%x, Reg[0x%x]=0x%x, Reg[0x%x]=0x%x\n",
		MT6350_AUXADC_LBAT3, upmu_get_reg_value(MT6350_AUXADC_LBAT3),
		MT6350_AUXADC_LBAT4, upmu_get_reg_value(MT6350_AUXADC_LBAT4),
		MT6350_INT_CON0, upmu_get_reg_value(MT6350_INT_CON0)
	    );
#endif

}

void bat_l_int_handler(void)
{
	PMICLOG_DBG("[bat_l_int_handler]....\n");

	/* sub-task */
#ifdef LOW_BATTERY_PROTECT
	g_low_battery_level++;
	if (g_low_battery_level > 2)
		g_low_battery_level = 2;

	if (g_low_battery_level == 1)
		exec_low_battery_callback(LOW_BATTERY_LEVEL_1);
	else if (g_low_battery_level == 2)
		exec_low_battery_callback(LOW_BATTERY_LEVEL_2);
	else
		PMICLOG("[bat_l_int_handler]err,g_low_battery_level=%d\n", g_low_battery_level);

#if 0
	lbat_min_en_setting(0);
	mdelay(1);
	lbat_max_en_setting(1);
#else

	pmic_set_register_value(PMIC_AUXADC_LBAT_VOLT_MIN, BAT_LV_2_THD);

	lbat_min_en_setting(0);
	lbat_max_en_setting(0);
	mdelay(1);
	if (g_low_battery_level < 2)
		lbat_min_en_setting(1);
	lbat_max_en_setting(1);
#endif

	PMICLOG("Reg[0x%x]=0x%x, Reg[0x%x]=0x%x, Reg[0x%x]=0x%x\n",
		MT6350_AUXADC_LBAT3, upmu_get_reg_value(MT6350_AUXADC_LBAT3),
		MT6350_AUXADC_LBAT4, upmu_get_reg_value(MT6350_AUXADC_LBAT4),
		MT6350_INT_CON0, upmu_get_reg_value(MT6350_INT_CON0)
	    );
#endif

}

/*
 * Battery OC call back function
 */
#define OCCB_NUM 16

#define DISABLE_BATTERY_OC_PROTECT
#ifndef DISABLE_BATTERY_OC_PROTECT
#define BATTERY_OC_PROTECT
#endif

#ifdef BATTERY_OC_PROTECT
/* ex. Ireg = 65535 - (I * 950000uA / 2 / 158.122 / CAR_TUNE_VALUE * 100) */
/* (950000/2/158.122)*100~=300400 */

/* ex: 4670mA */
#define BAT_OC_H_THD   (65535-((300400*POWER_BAT_OC_CURRENT_H/1000)/CAR_TUNE_VALUE))

/* ex: 5500mA */
#define BAT_OC_L_THD   (65535-((300400*POWER_BAT_OC_CURRENT_L/1000)/CAR_TUNE_VALUE))

/* ex: 3400mA */
#define BAT_OC_H_THD_RE   (65535-((300400*POWER_BAT_OC_CURRENT_H_RE/1000)/CAR_TUNE_VALUE))

/* ex: 4000mA */
#define BAT_OC_L_THD_RE   (65535-((300400*POWER_BAT_OC_CURRENT_L_RE/1000)/CAR_TUNE_VALUE))

int g_battery_oc_level = 0;
int g_battery_oc_stop = 0;

struct battery_oc_callback_table {
	void *occb;
};

struct battery_oc_callback_table occb_tb[] = {
	{NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL},
	{NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}
};

void (*battery_oc_callback)(BATTERY_OC_LEVEL);

void register_battery_oc_notify(void (*battery_oc_callback) (BATTERY_OC_LEVEL),
				BATTERY_OC_PRIO prio_val)
{
	PMICLOG("[register_battery_oc_notify] start\n");

	occb_tb[prio_val].occb = battery_oc_callback;

	PMICLOG("[register_battery_oc_notify] prio_val=%d\n", prio_val);
}

/* 0:no limit */
void exec_battery_oc_callback(BATTERY_OC_LEVEL battery_oc_level)
{
	int i = 0;

	if (g_battery_oc_stop == 1) {
		PMICLOG("[exec_battery_oc_callback] g_battery_oc_stop=%d\n", g_battery_oc_stop);
	} else {
		for (i = 0; i < OCCB_NUM; i++) {
			if (occb_tb[i].occb != NULL) {
				battery_oc_callback = occb_tb[i].occb;
				battery_oc_callback(battery_oc_level);
				PMICLOG
				    ("[exec_battery_oc_callback] prio_val=%d,battery_oc_level=%d\n",
				     i, battery_oc_level);
			}
		}
	}
}

void bat_oc_h_en_setting(int en_val)
{
	/* mt6325_upmu_set_rg_int_en_fg_cur_h(en_val); */
	pmic_set_register_value(PMIC_RG_INT_EN_FG_CUR_H, en_val);
}

void bat_oc_l_en_setting(int en_val)
{
	/* mt6325_upmu_set_rg_int_en_fg_cur_l(en_val); */
	pmic_set_register_value(PMIC_RG_INT_EN_FG_CUR_L, en_val);
}

void battery_oc_protect_init(void)
{
	pmic_set_register_value(PMIC_FG_CUR_HTH, BAT_OC_H_THD);	/* mt6325_upmu_set_fg_cur_hth(BAT_OC_H_THD); */
	pmic_set_register_value(PMIC_FG_CUR_LTH, BAT_OC_L_THD);	/* mt6325_upmu_set_fg_cur_lth(BAT_OC_L_THD); */

	bat_oc_h_en_setting(0);
	bat_oc_l_en_setting(1);

	PMICLOG("Reg[0x%x]=0x%x, Reg[0x%x]=0x%x, Reg[0x%x]=0x%x\n",
		MT6350_FGADC_CON23, upmu_get_reg_value(MT6350_FGADC_CON23),
		MT6350_FGADC_CON24, upmu_get_reg_value(MT6350_FGADC_CON24),
		MT6350_INT_CON2, upmu_get_reg_value(MT6350_INT_CON2)
	    );

	PMICLOG("[battery_oc_protect_init] %d mA, %d mA\n",
		POWER_BAT_OC_CURRENT_H, POWER_BAT_OC_CURRENT_L);
	PMICLOG("[battery_oc_protect_init] Done\n");
}

#endif				/* #ifdef BATTERY_OC_PROTECT */

void battery_oc_protect_reinit(void)
{
#ifdef BATTERY_OC_PROTECT
	/* mt6325_upmu_set_fg_cur_hth(BAT_OC_H_THD_RE); */
	pmic_set_register_value(PMIC_FG_CUR_HTH, BAT_OC_H_THD_RE);
	/* mt6325_upmu_set_fg_cur_lth(BAT_OC_L_THD_RE); */
	pmic_set_register_value(PMIC_FG_CUR_LTH, BAT_OC_L_THD_RE);

	PMICLOG("Reg[0x%x]=0x%x, Reg[0x%x]=0x%x, Reg[0x%x]=0x%x\n",
		MT6350_FGADC_CON23, upmu_get_reg_value(MT6350_FGADC_CON23),
		MT6350_FGADC_CON24, upmu_get_reg_value(MT6350_FGADC_CON24),
		MT6350_INT_CON2, upmu_get_reg_value(MT6350_INT_CON2)
	    );

	PMICLOG("[battery_oc_protect_reinit] %d mA, %d mA\n",
		POWER_BAT_OC_CURRENT_H_RE, POWER_BAT_OC_CURRENT_L_RE);
	PMICLOG("[battery_oc_protect_reinit] Done\n");
#else
	PMICLOG("[battery_oc_protect_reinit] no define BATTERY_OC_PROTECT\n");
#endif
}

void fg_cur_h_int_handler(void)
{
	PMICLOG_DBG("[fg_cur_h_int_handler]....\n");

	/* sub-task */
#ifdef BATTERY_OC_PROTECT
	g_battery_oc_level = 0;
	exec_battery_oc_callback(BATTERY_OC_LEVEL_0);
	bat_oc_h_en_setting(0);
	bat_oc_l_en_setting(0);
	mdelay(1);
	bat_oc_l_en_setting(1);

	PMICLOG("Reg[0x%x]=0x%x, Reg[0x%x]=0x%x, Reg[0x%x]=0x%x\n",
		MT6350_FGADC_CON23, upmu_get_reg_value(MT6350_FGADC_CON23),
		MT6350_FGADC_CON24, upmu_get_reg_value(MT6350_FGADC_CON24),
		MT6350_INT_CON2, upmu_get_reg_value(MT6350_INT_CON2)
	    );
#endif
}

void fg_cur_l_int_handler(void)
{
	PMICLOG_DBG("[fg_cur_l_int_handler]....\n");

	/* sub-task */
#ifdef BATTERY_OC_PROTECT
	g_battery_oc_level = 1;
	exec_battery_oc_callback(BATTERY_OC_LEVEL_1);
	bat_oc_h_en_setting(0);
	bat_oc_l_en_setting(0);
	mdelay(1);
	bat_oc_h_en_setting(1);

	PMICLOG("Reg[0x%x]=0x%x, Reg[0x%x]=0x%x, Reg[0x%x]=0x%x\n",
		MT6350_FGADC_CON23, upmu_get_reg_value(MT6350_FGADC_CON23),
		MT6350_FGADC_CON24, upmu_get_reg_value(MT6350_FGADC_CON24),
		MT6350_INT_CON2, upmu_get_reg_value(MT6350_INT_CON2)
	    );
#endif
}

/*
 * 15% notify service
 */
/* #ifndef DISABLE_BATTERY_PERCENT_PROTECT TBD */
#if 0
#define BATTERY_PERCENT_PROTECT
#endif

#ifdef BATTERY_PERCENT_PROTECT
static struct hrtimer bat_percent_notify_timer;
static struct task_struct *bat_percent_notify_thread;
static bool bat_percent_notify_flag;
static DECLARE_WAIT_QUEUE_HEAD(bat_percent_notify_waiter);

#ifdef CONFIG_PM_WAKELOCKS
struct wakeup_source bat_percent_notify_lock;
#else
struct wake_lock bat_percent_notify_lock;
#endif

static DEFINE_MUTEX(bat_percent_notify_mutex);

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

void (*battery_percent_callback)(BATTERY_PERCENT_LEVEL);

void register_battery_percent_notify(void (*battery_percent_callback) (BATTERY_PERCENT_LEVEL),
				     BATTERY_PERCENT_PRIO prio_val)
{
	PMICLOG("[register_battery_percent_notify] start\n");

	bpcb_tb[prio_val].bpcb = battery_percent_callback;

	PMICLOG("[register_battery_percent_notify] prio_val=%d\n", prio_val);

	if ((g_battery_percent_stop == 0) && (g_battery_percent_level == 1)) {
		PMICLOG("[register_battery_percent_notify] level l happen\n");
		battery_percent_callback(BATTERY_PERCENT_LEVEL_1);
	}
}

/* 0:no limit */
void exec_battery_percent_callback(BATTERY_PERCENT_LEVEL battery_percent_level)
{
	int i = 0;

	if (g_battery_percent_stop == 1) {
		PMICLOG("[exec_battery_percent_callback] g_battery_percent_stop=%d\n",
			g_battery_percent_stop);
	} else {
		for (i = 0; i < BPCB_NUM; i++) {
			if (bpcb_tb[i].bpcb != NULL) {
				battery_percent_callback = bpcb_tb[i].bpcb;
				battery_percent_callback(battery_percent_level);
				PMICLOG
				    ("[exec_battery_percent_callback] prio_val=%d,battery_percent_level=%d\n",
				     i, battery_percent_level);
			}
		}
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

#ifdef CONFIG_PM_WAKELOCKS
		__pm_stay_awake(&bat_percent_notify_lock);
#else
		wake_lock(&bat_percent_notify_lock);
#endif


		mutex_lock(&bat_percent_notify_mutex);

		bat_per_val = bat_get_ui_percentage();

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
#ifdef CONFIG_PM_WAKELOCKS
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
	PMICLOG_DBG("bat_percent_notify_task is called\n");

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
		PMICLOG("Failed to create bat_percent_notify_thread\n");
	else
		PMICLOG("Create bat_percent_notify_thread : done\n");
}
#endif				/* End of #ifdef BATTERY_PERCENT_PROTECT */

/*
 * DLPT service
 */
/* #ifndef DISABLE_DLPT_FEATURE  TBD */
#if 0
#define DLPT_FEATURE_SUPPORT
#endif

#ifdef DLPT_FEATURE_SUPPORT
static struct hrtimer dlpt_notify_timer;
static struct task_struct *dlpt_notify_thread;
static bool dlpt_notify_flag;
static DECLARE_WAIT_QUEUE_HEAD(dlpt_notify_waiter);

#ifdef CONFIG_PM_WAKELOCKS
struct wakeup_source dlpt_notify_lock;
#else
struct wake_lock dlpt_notify_lock;
#endif

static DEFINE_MUTEX(dlpt_notify_mutex);

#define DLPT_NUM 16


int g_dlpt_stop = 0;
unsigned int g_dlpt_val = 0;

int g_dlpt_start = 0;

int g_iavg_val = 0;
int g_imax_val = 0;
int g_imax_val_pre = 0;

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

	PMICLOG("[register_dlpt_notify] prio_val=%d\n", prio_val);

	if ((g_dlpt_stop == 0) && (g_dlpt_val != 0)) {
		PMICLOG("[register_dlpt_notify] dlpt happen\n");
		dlpt_callback(g_dlpt_val);
	}
}

void exec_dlpt_callback(unsigned int dlpt_val)
{
	int i = 0;

	g_dlpt_val = dlpt_val;

	if (g_dlpt_stop == 1) {
		PMICLOG("[exec_dlpt_callback] g_dlpt_stop=%d\n", g_dlpt_stop);
	} else {
		for (i = 0; i < DLPT_NUM; i++) {
			if (dlpt_cb_tb[i].dlpt_cb != NULL) {
				dlpt_callback = dlpt_cb_tb[i].dlpt_cb;
				dlpt_callback(g_dlpt_val);
				PMICLOG("[exec_dlpt_callback] g_dlpt_val=%d\n", g_dlpt_val);
			}
		}
	}
}

extern unsigned int bat_get_ui_percentage(void);
extern signed int fgauge_read_v_by_d(int d_val);
extern signed int fgauge_read_r_bat_by_v(signed int voltage);

int get_dlpt_iavg(int is_use_zcv)
{
	int bat_cap_val = 0;
	int zcv_val = 0;
	int vsys_min_2_val = POWER_INT2_VOLT;
	int rbat_val = 0;
	int rdc_val = 0;
	int iavg_val = 0;

	bat_cap_val = bat_get_ui_percentage();

	if (is_use_zcv == 1)
		zcv_val = fgauge_read_v_by_d(100 - bat_cap_val);
	else {
#if defined(SWCHR_POWER_PATH)
		zcv_val = PMIC_IMM_GetOneChannelValue(MT6350_AUX_ISENSE_AP, 5, 1);
#else
		zcv_val = PMIC_IMM_GetOneChannelValue(MT6350_AUX_BATSNS_AP, 5, 1);
#endif
	}
	rbat_val = fgauge_read_r_bat_by_v(zcv_val);
	rdc_val = CUST_R_FG_OFFSET + R_FG_VALUE + rbat_val;

	if (rdc_val == 0)
		rdc_val = 1;
	iavg_val = ((zcv_val - vsys_min_2_val) * 1000) / rdc_val;	/* ((mV)*1000)/m-ohm */

	PMICLOG
	    ("[dlpt] SOC=%d,ZCV=%d,vsys_min_2=%d,rpcb=%d,rfg=%d,rbat=%d,rdc=%d,iavg=%d(mA),is_use_zcv=%d\n",
	     bat_cap_val, zcv_val, vsys_min_2_val, CUST_R_FG_OFFSET, R_FG_VALUE, rbat_val, rdc_val,
	     iavg_val, is_use_zcv);
	PMICLOG("[dlpt_Iavg] %d,%d,%d,%d,%d,%d,%d,%d,%d\n", bat_cap_val, zcv_val, vsys_min_2_val,
		CUST_R_FG_OFFSET, R_FG_VALUE, rbat_val, rdc_val, iavg_val, is_use_zcv);
	return iavg_val;
}

int get_real_volt(int val)
{				/* 0.1mV */
	int ret = 0;

	ret = val & 0x7FFF;
	ret = (ret * 4 * 1800 * 10) / 32768;
	return ret;
}

int get_real_curr(int val)
{				/* 0.1mA */
	int ret = 0;

	if (val > 32767) {	/* > 0x8000 */
		ret = val - 65535;
		ret = ret - (ret * 2);
	} else
		ret = val;
	ret = ret * 158122;
	do_div(ret, 100000);
	ret = (ret * 20) / R_FG_VALUE;
	ret = ((ret * CAR_TUNE_VALUE) / 100);

	return ret;
}

int get_rac_val(void)
{
#if 0
	PMICLOG("[get_rac_val] TBD\n");
	return 1;
#else
	int volt_1 = 0;
	int volt_2 = 0;
	int curr_1 = 0;
	int curr_2 = 0;
	int rac_cal = 0;
	int ret = 0;
	bool retry_state = false;
	int retry_count = 0;

	do {
		/* adc and fg-------------------------------------------------------- */
#if defined(SWCHR_POWER_PATH)
		volt_1 = PMIC_IMM_GetOneChannelValue(MT6328_AUX_ISENSE_AP, 5, 10);
#else
		volt_1 = PMIC_IMM_GetOneChannelValue(MT6328_AUX_BATSNS_AP, 5, 10);
#endif
		curr_1 = battery_meter_get_battery_current();
		PMICLOG("[1,Trigger ADC PTIM mode] volt_1=%d, curr_1=%d\n", volt_1, curr_1);

		/* Enable dummy load-------------------------------------------------- */
		pmic_config_interface(0x23C, 0x40, 0xFFFF, 0);	/* Reg[0x238][6]=0 */
		/* mt6325_upmu_set_rg_g_drv_2m_ck_pdn(0); */
		/* mt6325_upmu_set_rg_drv_32k_ck_pdn(0); */
		/* mt6325_upmu_set_rg_drv_isink2_ck_cksel(0); */
		/* mt6325_upmu_set_isink_ch2_mode(3); */
		/* mt6325_upmu_set_isink_chop2_en(1); */
		/* mt6325_upmu_set_isink_ch2_bias_en(1); */
		/* mt6325_upmu_set_isink_ch2_step(7); */
		/* mt6325_upmu_set_rg_isink2_double_en(1); */
		/* mt6325_upmu_set_isink_ch2_en(1); */

		PMICLOG
		    ("[2,Enable dummy load] [0x%x]=0x%x,[0x%x]=0x%x,[0x%x]=0x%x,[0x%x]=0x%x,[0x%x]=0x%x,[0x%x]=0x%x\n",
		     MT6328_TOP_CKPDN_CON0, upmu_get_reg_value(MT6328_TOP_CKPDN_CON0),
		     MT6328_TOP_CKSEL_CON0, upmu_get_reg_value(MT6328_TOP_CKSEL_CON0),
		     MT6328_ISINK_MODE_CTRL, upmu_get_reg_value(MT6328_ISINK_MODE_CTRL),
		     MT6328_ISINK_EN_CTRL, upmu_get_reg_value(MT6328_ISINK_EN_CTRL),
		     MT6328_ISINK2_CON1, upmu_get_reg_value(MT6328_ISINK2_CON1), MT6328_ISINK_ANA0,
		     upmu_get_reg_value(MT6328_ISINK_ANA0)
		    );

		/* Wait -------------------------------------------------------------- */
		msleep(50);

		/* adc and fg-------------------------------------------------------- */
#if defined(SWCHR_POWER_PATH)
		volt_2 = PMIC_IMM_GetOneChannelValue(MT6328_AUX_ISENSE_AP, 5, 10);
#else
		volt_2 = PMIC_IMM_GetOneChannelValue(MT6328_AUX_BATSNS_AP, 5, 10);
#endif
		curr_2 = battery_meter_get_battery_current();
		PMICLOG("[3,Trigger ADC PTIM mode] volt_1=%d, curr_1=%d\n", volt_2, curr_2);

		/* Disable dummy load------------------------------------------------- */
		/* mt6325_upmu_set_isink_ch2_en(0); */
		/* //pmic_config_interface(0x23A,0x40,0xFFFF,0); //Reg[0x238][6]=1, move to suspend callback */

		/* Calculate Rac------------------------------------------------------ */
		if ((curr_2 - curr_1) >= 400) {	/* 40.0mA */
			rac_cal = ((volt_1 - volt_2) * 1000) / (curr_2 - curr_1);	/* m-ohm */

			if (rac_cal < 0)
				ret = rac_cal - (rac_cal * 2);
			else
				ret = rac_cal;
		} else if ((curr_1 - curr_2) >= 400) {	/* 40.0mA */
			rac_cal = ((volt_2 - volt_1) * 1000) / (curr_1 - curr_2);	/* m-ohm */

			if (rac_cal < 0)
				ret = rac_cal - (rac_cal * 2);
			else
				ret = rac_cal;
		} else {
			ret = -1;
			PMICLOG("[5,Calculate Rac] bypass due to (curr_x-curr_y) < 40mA\n");
		}

		PMICLOG
		    ("[5,Calculate Rac] volt_1=%d,volt_2=%d,curr_1=%d,curr_2=%d,rac_cal=%d,ret=%d,retry_count=%d\n",
		     volt_1, volt_2, curr_1, curr_2, rac_cal, ret, retry_count);

		/* ------------------------ */
		retry_count++;

		if ((retry_count < 3) && (ret == -1))
			retry_state = true;
		else
			retry_state = false;

	} while (retry_state == true);

	return ret;
#endif
}

int get_dlpt_imax(void)
{
	int bat_cap_val = 0;
	int zcv_val = 0;
	int vsys_min_1_val = POWER_INT1_VOLT;
	int rac_val = 0;
	int imax_val = 0;

	bat_cap_val = bat_get_ui_percentage();
	zcv_val = fgauge_read_v_by_d(100 - bat_cap_val);

	rac_val = get_rac_val();

	if (rac_val == -1)
		return -1;

	if (rac_val == 0)
		rac_val = 1;
	imax_val = ((zcv_val - vsys_min_1_val) * 1000) / rac_val;	/* ((mV)*1000)/m-ohm */

	PMICLOG("[dlpt] SOC=%d,ZCV=%d,vsys_min_1=%d,rac=%d,imax=%d(mA)\n",
		bat_cap_val, zcv_val, vsys_min_1_val, rac_val, imax_val);
	PMICLOG("[dlpt_Imax] %d,%d,%d,%d,%d\n",
		bat_cap_val, zcv_val, vsys_min_1_val, rac_val, imax_val);
	return imax_val;
}

int dlpt_check_power_off(void)
{
	int ret = 0;

	ret = 0;
	if (g_dlpt_start == 0) {
		PMICLOG("[dlpt_check_power_off] not start\n");
	} else {
		if (!upmu_get_rgs_chrdet()) {
			if ((g_iavg_val < POWEROFF_BAT_CURRENT) && (g_low_battery_level > 0))
				ret = 1;
			else if (g_low_battery_level == 2)
				ret = 1;
			else
				ret = 0;

			PMICLOG
			    ("[pw_off_chk] g_iavg_val=%d,POWEROFF_BAT_CURRENT=%d,g_low_battery_level=%d, ret=%d\n",
			     g_iavg_val, POWEROFF_BAT_CURRENT, g_low_battery_level, ret);
		}
	}

	return ret;
}

int get_dlpt_imax_avg(void)
{
	int i = 0;
	int val[5] = { 0, 0, 0, 0, 0 };
	int val_compare = g_iavg_val * 3;
	int timeout_i = 0;
	int timeout_val = 100;

	for (i = 0; i < 5; i++) {
		val[i] = get_dlpt_imax();
		if ((val[i] > val_compare) || (val[i] < g_iavg_val) || (val[i] <= 0))
			i--;	/* re-measure */

		timeout_i++;
		if (timeout_i >= timeout_val) {
			PMICLOG("[get_dlpt_imax_avg] timeout, return -1\n");
			return -1;
		}
	}

	PMICLOG("[get_dlpt_imax_avg] %d,%d,%d,%d,%d,i=%d,val_compare=%d,timeout_i=%d\n",
		val[0], val[1], val[2], val[3], val[4], i, val_compare, timeout_i);

	if (i <= 0)
		return -1;
	else
		return (val[0] + val[1] + val[2] + val[3] + val[4]) / 5;
}

int dlpt_notify_handler(void *unused)
{
	ktime_t ktime;
	int pre_ui_soc = 0;
	int cur_ui_soc = 0;
	int diff_ui_soc = 1;

	pre_ui_soc = bat_get_ui_percentage();
	cur_ui_soc = pre_ui_soc;

	do {
		ktime = ktime_set(10, 0);

		wait_event_interruptible(dlpt_notify_waiter, (dlpt_notify_flag == true));

		wake_lock(&dlpt_notify_lock);
		mutex_lock(&dlpt_notify_mutex);
		/* --------------------------------- */

		cur_ui_soc = bat_get_ui_percentage();
		/* if(1) */
		if (((pre_ui_soc - cur_ui_soc) >= diff_ui_soc) ||
		    ((cur_ui_soc - pre_ui_soc) >= diff_ui_soc) ||
		    (g_imax_val == 0) || (g_imax_val == -1)
		    ) {
			if (upmu_get_rgs_chrdet()) {
				g_iavg_val = get_dlpt_iavg(0);
				g_imax_val = (g_iavg_val * 125) / 100;

				/* sync value */
				g_imax_val_pre = g_imax_val;
			} else {
				g_iavg_val = get_dlpt_iavg(1);
				g_imax_val = get_dlpt_imax_avg();

				/* sync value */
				if (g_imax_val != -1) {
					if (g_imax_val_pre <= 0)
						g_imax_val_pre = g_imax_val;
					if (g_imax_val > g_imax_val_pre) {
						PMICLOG("[DLPT] g_imax_val=%d,g_imax_val_pre=%d\n",
							g_imax_val, g_imax_val_pre);
						g_imax_val = g_imax_val_pre;
					} else {
						g_imax_val_pre = g_imax_val;
					}
				}
			}

			/* 10/21, Ricky, report imax=iavg*1.25 */
			/* g_imax_val=(g_iavg_val*125)/100; */
			/* PMICLOG("[DLPT] Ricky(imax=%d,iavg=%d)\n", g_imax_val, g_iavg_val); */

			/* Notify */
			if (g_imax_val != -1) {
				if (g_imax_val > IMAX_MAX_VALUE)
					g_imax_val = IMAX_MAX_VALUE;

				exec_dlpt_callback(g_imax_val);
			} else {
				PMICLOG("[DLPT] bad value, ignore DLPT\n");
			}

			pre_ui_soc = cur_ui_soc;

			PMICLOG("[DLPT_final] %d,%d,%d,%d,%d,%d,%d\n",
				g_iavg_val, g_imax_val_pre, pre_ui_soc, cur_ui_soc, diff_ui_soc,
				IMAX_MAX_VALUE, g_imax_val);
		}

		g_dlpt_start = 1;
		dlpt_notify_flag = KAL_FALSE;

		/* --------------------------------- */
		mutex_unlock(&dlpt_notify_mutex);
		wake_unlock(&dlpt_notify_lock);

		hrtimer_start(&dlpt_notify_timer, ktime, HRTIMER_MODE_REL);

	} while (!kthread_should_stop());

	return 0;
}

enum hrtimer_restart dlpt_notify_task(struct hrtimer *timer)
{
	dlpt_notify_flag = true;
	wake_up_interruptible(&dlpt_notify_waiter);
	PMICLOG_DBG("dlpt_notify_task is called\n");

	return HRTIMER_NORESTART;
}

int get_system_loading_ma(void)
{
	int fg_val = 0;

	if (g_dlpt_start == 0)
		PMICLOG("get_system_loading_ma not ready\n");
	else {
		fg_val = battery_meter_get_battery_current();
		fg_val = fg_val / 10;
		if (battery_meter_get_battery_current_sign() == 1)
			fg_val = 0 - fg_val;	/* charging */
		PMICLOG("[get_system_loading_ma] fg_val=%d\n", fg_val);
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
		PMICLOG("Failed to create dlpt_notify_thread\n");
	else
		PMICLOG("Create dlpt_notify_thread : done\n");

	pmic_set_register_value(PMIC_RG_UVLO_VTHL, 0);

	/* re-init UVLO volt */
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
	PMICLOG("POWER_UVLO_VOLT_LEVEL=%d, [0x%x]=0x%x\n",
		POWER_UVLO_VOLT_LEVEL, MT6350_CHR_CON17, upmu_get_reg_value(MT6350_CHR_CON17));
}

#endif				/* #ifdef DLPT_FEATURE_SUPPORT */

/*
 * interrupt Setting
 */
static struct pmic_interrupt_bit interrupt_status0[] = {
	PMIC_S_INT_GEN(RG_INT_STATUS_SPKL_AB),
	PMIC_S_INT_GEN(RG_INT_STATUS_SPKL),
	PMIC_S_INT_GEN(RG_INT_STATUS_BAT_L),
	PMIC_S_INT_GEN(RG_INT_STATUS_BAT_H),
	PMIC_S_INT_GEN(RG_INT_STATUS_WATCHDOG),
	PMIC_S_INT_GEN(RG_INT_STATUS_PWRKEY),
	PMIC_S_INT_GEN(RG_INT_STATUS_THR_L),
	PMIC_S_INT_GEN(RG_INT_STATUS_THR_H),
	PMIC_S_INT_GEN(RG_INT_STATUS_VBATON_UNDET),
	PMIC_S_INT_GEN(RG_INT_STATUS_BVALID_DET),
	PMIC_S_INT_GEN(RG_INT_STATUS_CHRDET),
	PMIC_S_INT_GEN(RG_INT_STATUS_OV),
	PMIC_S_INT_GEN(NO_USE),
	PMIC_S_INT_GEN(NO_USE),
	PMIC_S_INT_GEN(NO_USE),
	PMIC_S_INT_GEN(NO_USE),
};

static struct pmic_interrupt_bit interrupt_status1[] = {
	PMIC_S_INT_GEN(RG_INT_STATUS_LDO),
	PMIC_S_INT_GEN(RG_INT_STATUS_FCHRKEY),
	PMIC_S_INT_GEN(RG_INT_STATUS_ACCDET),
	PMIC_S_INT_GEN(RG_INT_STATUS_AUDIO),
	PMIC_S_INT_GEN(RG_INT_STATUS_RTC),
	PMIC_S_INT_GEN(RG_INT_STATUS_VPROC),
	PMIC_S_INT_GEN(RG_INT_STATUS_VSYS),
	PMIC_S_INT_GEN(RG_INT_STATUS_VPA),
	PMIC_S_INT_GEN(NO_USE),
	PMIC_S_INT_GEN(NO_USE),
	PMIC_S_INT_GEN(NO_USE),
	PMIC_S_INT_GEN(NO_USE),
	PMIC_S_INT_GEN(NO_USE),
	PMIC_S_INT_GEN(NO_USE),
	PMIC_S_INT_GEN(NO_USE),
	PMIC_S_INT_GEN(NO_USE),
};



static struct pmic_interrupts interrupts[] = {
	PMIC_M_INTS_GEN(MT6350_INT_STATUS0, MT6350_INT_CON0, MT6350_INT_CON0_SET,
			MT6350_INT_CON0_CLR, interrupt_status0),
	PMIC_M_INTS_GEN(MT6350_INT_STATUS1, MT6350_INT_CON1, MT6350_INT_CON1_SET,
			MT6350_INT_CON1_CLR, interrupt_status1),
};

void pwrkey_int_handler(void)
{
#if defined(CONFIG_MTK_KERNEL_POWER_OFF_CHARGING_FIX)
	static bool key_press;
#endif

	PMICLOG_DBG("[pwrkey_int_handler] Press pwrkey %d\n", pmic_get_register_value(PMIC_PWRKEY_DEB));
	if (pmic_get_register_value(PMIC_PWRKEY_DEB) == 1) {
#if defined(CONFIG_MTK_KERNEL_POWER_OFF_CHARGING_FIX)
		if (g_boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT) {
			if (key_press == true) {
				key_press = false;
				timer_pos = sched_clock();
				PMICLOG("[pmic_thread_kthread] timer_pos = %ld\r\n", timer_pos);
				if (timer_pos - timer_pre >= LONG_PWRKEY_PRESS_TIME)
					long_pwrkey_press = true;

				PMICLOG("timer_pos = %ld, timer_pre = %ld, timer_pos-timer_pre = %ld\n"
					 timer_pos, timer_pre, timer_pos - timer_pre);
				PMICLOG("long_pwrkey_press = %d\r\n", long_pwrkey_press);
				if (long_pwrkey_press) {	/* 500ms */
					PMICLOG
					    ("Power Key Pressed during kernel power off charging, reboot OS\r\n");
					/*TODO arch_reset(0, NULL);*/
				}
			}
		}
#endif
#ifdef CONFIG_KEYBOARD_MTK
		kpd_pwrkey_pmic_handler(0x0);
#endif
		pmic_set_register_value(PMIC_RG_PWRKEY_INT_SEL, 0);
	} else {
		PMICLOG("[pwrkey_int_handler] Press pwrkey\n");
#if defined(CONFIG_MTK_KERNEL_POWER_OFF_CHARGING_FIX)
		if (g_boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT) {
			key_press = KAL_TRUE;
			timer_pre = sched_clock();
			PMICLOG("[pmic_thread_kthread] timer_pre = %ld, \r\n", timer_pre);
		}
#endif
#ifdef CONFIG_KEYBOARD_MTK
		kpd_pwrkey_pmic_handler(0x1);
#endif
		pmic_set_register_value(PMIC_RG_PWRKEY_INT_SEL, 1);
	}
	/* ret = pmic_config_interface(MT6350_INT_STATUS0, 0x1, 0x1, 0x5); */
}

/*
 * PMIC Interrupt callback
 */
#ifndef CONFIG_KEYBOARD_MTK
void kpd_pmic_rstkey_handler(unsigned long pressed)
{
}
#endif

void homekey_int_handler(void)
{
	PMICLOG_DBG("[homekey_int_handler] Press homekey %d\n",
		pmic_get_register_value(PMIC_FCHRKEY_DEB));

#ifdef KPD_PMIC_RSTKEY_MAP
	if (pmic_get_register_value(PMIC_FCHRKEY_DEB) == 1) {
		PMICLOG("[homekey_int_handler] Release HomeKey\r\n");
		kpd_pmic_rstkey_handler(0x0);
	} else {
		PMICLOG("[homekey_int_handler] Press HomeKey\r\n");
		kpd_pmic_rstkey_handler(0x1);
	}
#else
	PMICLOG("[fchr_key_int_handler]....\n");
#endif
}

void chrdet_int_handler(void)
{
	PMICLOG_DBG("[chrdet_int_handler]CHRDET status = %d....\n",
		pmic_get_register_value(PMIC_RGS_CHRDET));

#ifdef CONFIG_MTK_KERNEL_POWER_OFF_CHARGING
	if (!upmu_get_rgs_chrdet()) {
		int boot_mode = 0;

		boot_mode = get_boot_mode();

		if (boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT
		    || boot_mode == LOW_POWER_OFF_CHARGING_BOOT) {
			PMICLOG("[chrdet_int_handler] Unplug Charger/USB\n");
			mt_power_off();
		}
	}
#endif
	pmic_set_register_value(PMIC_RG_USBDL_RST, 1);
#ifdef CONFIG_MTK_SMART_BATTERY
	do_chrdet_int_task();
#endif
}

#ifdef CONFIG_MTK_ACCDET
void accdet_int_handler(void)
{
	unsigned int ret = 0;

	PMICLOG_DBG("[accdet_int_handler]....\n");

	ret = accdet_irq_handler();
	if (0 == ret)
		PMICLOG("[accdet_int_handler] don't finished\n");
/* ret=pmic_config_interface(INT_STATUS1,0x1,0x1,2); */
}
#endif

#ifdef CONFIG_MTK_RTC
void rtc_int_handler(void)
{
	PMICLOG_DBG("[rtc_int_handler]....\n");
#ifndef CONFIG_EARLY_LINUX_PORTING
	rtc_irq_handler();
#endif
	/* ret=pmic_config_interface(MT6350_INT_STATUS1, 0x1, 0x1, 4); */
}
#endif				/* End of #ifdef CONFIG_MTK_RTC */

void auxadc_imp_int_handler_r(void)
{
	/*
	   PMICLOG("auxadc_imp_int_handler_r() =%d\n",pmic_get_register_value(PMIC_AUXADC_ADC_OUT_IMP));
	 */
	/* clear IRQ */
	/*
	   pmic_set_register_value(PMIC_AUXADC_CLR_IMP_CNT_STOP,1);
	   pmic_set_register_value(PMIC_AUXADC_IMPEDANCE_IRQ_CLR,1);
	 */
	/*restore to initial state */
	/*
	   pmic_set_register_value(PMIC_AUXADC_CLR_IMP_CNT_STOP,0);
	   pmic_set_register_value(PMIC_AUXADC_IMPEDANCE_IRQ_CLR,0);
	 */
	/* turn off interrupt */
	/*
	   pmic_set_register_value(PMIC_RG_INT_EN_AUXADC_IMP,0);
	 */
}

/*
 * PMIC Interrupt service
 */
static DEFINE_MUTEX(pmic_mutex);
static struct task_struct *pmic_thread_handle;
#ifdef CONFIG_PM_WAKELOCKS
struct wakeup_source pmicThread_lock;
#else
struct wake_lock pmicThread_lock;
#endif

void wake_up_pmic(void)
{
	PMICLOG_DBG("[wake_up_pmic]\r\n");
	wake_up_process(pmic_thread_handle);

#ifdef CONFIG_PM_WAKELOCKS
	__pm_stay_awake(&pmicThread_lock);
#else
	wake_lock(&pmicThread_lock);
#endif
}
EXPORT_SYMBOL(wake_up_pmic);

#if 0 /* #ifdef CONFIG_MTK_LEGACY */
void mt_pmic_eint_irq(void)
{
	PMICLOG("[mt_pmic_eint_irq] receive interrupt\n");
	wake_up_pmic();
}
#else
irqreturn_t mt_pmic_eint_irq(int irq, void *desc)
{

	PMICLOG_DBG("[mt_pmic_eint_irq] receive interrupt\n");
	disable_irq_nosync(irq);
	wake_up_pmic();

	return IRQ_HANDLED;
}
#endif

void pmic_enable_interrupt(unsigned int intNo, unsigned int en, char *str)
{
	unsigned int shift, no;

	shift = intNo / PMIC_INT_WIDTH;
	no = intNo % PMIC_INT_WIDTH;

	if (shift >= ARRAY_SIZE(interrupts)) {
		PMICLOG("[pmic_enable_interrupt] fail intno=%d \r\n", intNo);
		return;
	}

	PMICLOG_DBG("[pmic_enable_interrupt] intno=%d en=%d str=%s shf=%d no=%d [0x%x]=0x%x\r\n", intNo,
		en, str, shift, no, interrupts[shift].en, upmu_get_reg_value(interrupts[shift].en));

	if (en == 1)
		pmic_config_interface(interrupts[shift].set, 0x1, 0x1, no);
	else if (en == 0)
		pmic_config_interface(interrupts[shift].clear, 0x1, 0x1, no);

	PMICLOG("[pmic_enable_interrupt] after [0x%x]=0x%x\r\n",
		interrupts[shift].en, upmu_get_reg_value(interrupts[shift].en));

}

void pmic_register_interrupt_callback(unsigned int intNo, void (EINT_FUNC_PTR) (void))
{
	unsigned int shift, no;

	shift = intNo / PMIC_INT_WIDTH;
	no = intNo % PMIC_INT_WIDTH;

	if (shift >= ARRAY_SIZE(interrupts)) {
		PMICLOG("[pmic_register_interrupt_callback] fail intno=%d \r\n", intNo);
		return;
	}

	PMICLOG_DBG("[pmic_register_interrupt_callback] intno=%d \r\n", intNo);

	interrupts[shift].interrupts[no].callback = EINT_FUNC_PTR;

}

int g_pmic_irq;
void PMIC_EINT_SETTING(void)
{
#ifdef CONFIG_OF
	int ret = 0;
	unsigned int ints[2] = { 0, 0 };
	struct device_node *node;
#endif
	upmu_set_reg_value(MT6350_INT_CON0, 0);
	upmu_set_reg_value(MT6350_INT_CON1, 0);

	/* Enable pwrkey/homekey interrupt */
	upmu_set_reg_value(MT6350_INT_CON0_SET, 0x0420);	/* bit[10], bit[5] */
	upmu_set_reg_value(MT6350_INT_CON1_SET, 0x0016);	/* bit[4], bit[2], bit[1] */

	/* for all interrupt events, turn on interrupt module clock */
	pmic_set_register_value(PMIC_RG_INTRP_CK_PDN, 0);

	pmic_register_interrupt_callback(5, pwrkey_int_handler);
	pmic_register_interrupt_callback(10, chrdet_int_handler);
	pmic_register_interrupt_callback(17, homekey_int_handler);
#ifdef CONFIG_MTK_ACCDET
	pmic_register_interrupt_callback(18, accdet_int_handler);
#endif
#ifdef CONFIG_MTK_RTC
	pmic_register_interrupt_callback(20, rtc_int_handler);
#endif

	pmic_enable_interrupt(5, 1, "PMIC");
	pmic_enable_interrupt(10, 1, "PMIC");
	pmic_enable_interrupt(17, 1, "PMIC");
#ifdef CONFIG_MTK_ACCDET
	pmic_enable_interrupt(18, 1, "PMIC");
#endif
	pmic_enable_interrupt(20, 1, "PMIC");

#if 0 /* #ifdef CONFIG_MTK_LEGACY */
/*
	mt_eint_set_hw_debounce(g_eint_pmic_num, g_cust_eint_mt_pmic_debounce_cn);
	mt_eint_registration(g_eint_pmic_num, g_cust_eint_mt_pmic_type, mt_pmic_eint_irq, 0);
	mt_eint_unmask(g_eint_pmic_num);
TBD */
#else
	node = of_find_compatible_node(NULL, NULL, "mediatek, pmic-eint");
	if (node) {
		of_property_read_u32_array(node, "debounce", ints, ARRAY_SIZE(ints));
		mt_gpio_set_debounce(ints[0], ints[1]);

		g_pmic_irq = irq_of_parse_and_map(node, 0);
		ret =
		    request_irq(g_pmic_irq, mt_pmic_eint_irq, IRQF_TRIGGER_NONE, "pmic-eint", NULL);
		if (ret > 0)
			PMICLOG("EINT IRQ LINENNOT AVAILABLE\n");

		enable_irq(g_pmic_irq);
		enable_irq_wake(g_pmic_irq);
	} else
		PMICLOG("%s can't find compatible node\n", __func__);
#endif
	PMICLOG("[CUST_EINT] CUST_EINT_MT_PMIC_MT6350_NUM=%d\n", g_eint_pmic_num);
	PMICLOG("[CUST_EINT] CUST_EINT_PMIC_DEBOUNCE_CN=%d\n", g_cust_eint_mt_pmic_debounce_cn);
	PMICLOG("[CUST_EINT] CUST_EINT_PMIC_TYPE=%d\n", g_cust_eint_mt_pmic_type);
	PMICLOG("[CUST_EINT] CUST_EINT_PMIC_DEBOUNCE_EN=%d\n", g_cust_eint_mt_pmic_debounce_en);

}

static void pmic_int_handler(void)
{
	unsigned char i, j;
	unsigned int ret;
	unsigned int int0, int1;

#if 1
	int0 = upmu_get_reg_value(MT6350_INT_CON0);
	int1 = upmu_get_reg_value(MT6350_INT_CON1);
	PMICLOG_DBG("int0 = %x int1 = %x\n", int0, int1);
	upmu_set_reg_value(MT6350_INT_CON0_CLR, 0x0210);	/* bit[9], bit[4] */

#endif

	for (i = 0; i < ARRAY_SIZE(interrupts); i++) {
		unsigned int int_status_val = 0;

		int_status_val = upmu_get_reg_value(interrupts[i].address);
		PMICLOG("[PMIC_INT] addr[0x%x]=0x%x\n", interrupts[i].address, int_status_val);

		for (j = 0; j < PMIC_INT_WIDTH; j++) {
			if ((int_status_val) & (1 << j)) {
				PMICLOG_DBG("[PMIC_INT][%s]\n", interrupts[i].interrupts[j].name);
				if (interrupts[i].interrupts[j].callback != NULL) {
					interrupts[i].interrupts[j].callback();
					interrupts[i].interrupts[j].times++;
				}
				ret = pmic_config_interface(interrupts[i].address, 0x1, 0x1, j);
			}
		}
	}
}

static int pmic_thread_kthread(void *x)
{
	unsigned int i;
	unsigned int int_status_val = 0;
	struct sched_param param = {.sched_priority = 98 };

	sched_setscheduler(current, SCHED_FIFO, &param);
	set_current_state(TASK_INTERRUPTIBLE);

	PMICLOG("[PMIC_INT] enter\n");

	/* Run on a process content */
	while (1) {
		mutex_lock(&pmic_mutex);

		pmic_int_handler();

		/* pmic_wrap_eint_clr(0x0); */
		/* PMICLOG("[PMIC_INT] pmic_wrap_eint_clr(0x0);\n"); */

		for (i = 0; i < ARRAY_SIZE(interrupts); i++) {
			int_status_val = upmu_get_reg_value(interrupts[i].address);
			PMICLOG_DBG("[PMIC_INT] after ,int_status_val[0x%x]=0x%x\n",
				interrupts[i].address, int_status_val);
		}


		mdelay(1);

		mutex_unlock(&pmic_mutex);
#ifdef CONFIG_PM_WAKELOCKS
		__pm_relax(&pmicThread_lock);
#else
		wake_unlock(&pmicThread_lock);
#endif

		set_current_state(TASK_INTERRUPTIBLE);
#if 0 /* #ifdef CONFIG_MTK_LEGACY */
#if 0				/* TBD */
		mt_eint_unmask(g_eint_pmic_num);	/* need fix */
#endif
#else
		if (g_pmic_irq != 0)
			enable_irq(g_pmic_irq);
#endif
		schedule();
	}

	return 0;
}

int pmic_is_ext_buck_exist(void)
{
	return 0;
}

int is_ext_buck2_exist(void)
{
/* return mt_get_gpio_in(140); */
	return 0;
}

int is_ext_vbat_boost_exist(void)
{
	return 0;
}

int is_ext_swchr_exist(void)
{
	return 0;
}

/*
 * FTM
 */
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
		/* #if defined(FTM_EXT_BUCK_CHECK) */
	case Get_IS_EXT_BUCK_EXIST:
		user_data_addr = (int *)arg;
		ret = copy_from_user(adc_in_data, user_data_addr, 8);
		adc_out_data[0] = pmic_is_ext_buck_exist();
		ret = copy_to_user(user_data_addr, adc_out_data, 8);
		PMICLOG("[pmic_ftm_ioctl] Get_IS_EXT_BUCK_EXIST:%d\n", adc_out_data[0]);
		break;
		/* #endif */

		/* #if defined(FTM_EXT_VBAT_BOOST_CHECK) */
	case Get_IS_EXT_VBAT_BOOST_EXIST:
		user_data_addr = (int *)arg;
		ret = copy_from_user(adc_in_data, user_data_addr, 8);
		adc_out_data[0] = is_ext_vbat_boost_exist();
		ret = copy_to_user(user_data_addr, adc_out_data, 8);
		PMICLOG("[pmic_ftm_ioctl] Get_IS_EXT_VBAT_BOOST_EXIST:%d\n", adc_out_data[0]);
		break;
		/* #endif */

		/* #if defined(FEATURE_FTM_SWCHR_HW_DETECT) */
	case Get_IS_EXT_SWCHR_EXIST:
		user_data_addr = (int *)arg;
		ret = copy_from_user(adc_in_data, user_data_addr, 8);
		adc_out_data[0] = is_ext_swchr_exist();
		ret = copy_to_user(user_data_addr, adc_out_data, 8);
		PMICLOG("[pmic_ftm_ioctl] Get_IS_EXT_SWCHR_EXIST:%d\n", adc_out_data[0]);
		break;
		/* #endif */
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

/*
 * HW Setting
 */
void PMIC_INIT_SETTING_V1(void)
{
	unsigned int chip_version = 0;
	unsigned int ret = 0;

	chip_version = pmic_get_register_value(PMIC_CID);

	PMICLOG("[Kernel_PMIC_INIT_SETTING_V1] 6350 PMIC Chip = 0x%x\n", chip_version);
	/* [7:4]: RG_VCDT_HV_VTH; 7V OVP, Tim,Zax */
	ret = pmic_config_interface(0x2, 0xB, 0xF, 4);
	/* [3:1]: RG_VBAT_OV_VTH; VBAT_OV=4.3V, Tim Zax, need to check Description */
	ret = pmic_config_interface(0xC, 0x1, 0x7, 1);
	/* [3:0]: RG_CHRWDT_TD; align to 6250's, Tim,zax */
	ret = pmic_config_interface(0x1A, 0x3, 0xF, 0);
	/* [2:2]: RG_USBDL_RST; Maxwell */
	ret = pmic_config_interface(0x20, 0x1, 0x1, 2);
	/* [1:1]: RG_BC11_RST; Reset BC11 detection, Tim,zax,charger programming guide */
	ret = pmic_config_interface(0x24, 0x1, 0x1, 1);
	/* [6:4]: RG_CSDAC_STP; align to 6250's setting,Tim, Zax */
	ret = pmic_config_interface(0x2A, 0x0, 0x7, 4);
	/* [2:2]: RG_CSDAC_MODE; Tim,Zax */
	ret = pmic_config_interface(0x2E, 0x1, 0x1, 2);
	/* [6:6]: RG_HWCV_EN; Align to MT6328,Top off set=1, TIM,Zax */
	ret = pmic_config_interface(0x2E, 0x0, 0x1, 6);
	/* [7:7]: RG_ULC_DET_EN; Tim,Zax */
	ret = pmic_config_interface(0x2E, 0x1, 0x1, 7);
	/* [4:4]: RG_EN_DRVSEL; Ricky */
	ret = pmic_config_interface(0x40, 0x1, 0x1, 4);
	/* [5:5]: RG_RST_DRVSEL; Ricky */
	ret = pmic_config_interface(0x40, 0x1, 0x1, 5);
	/* [1:1]: PWRBB_DEB_EN; Ricky */
	ret = pmic_config_interface(0x46, 0x1, 0x1, 1);
	/* [8:8]: VPROC_PG_H2L_EN; Ricky */
	ret = pmic_config_interface(0x48, 0x1, 0x1, 8);
	/* [9:9]: VSYS_PG_H2L_EN; Ricky */
	ret = pmic_config_interface(0x48, 0x1, 0x1, 9);
	/* [5:5]: STRUP_AUXADC_RSTB_SW; need to check with ABB */
	ret = pmic_config_interface(0x4E, 0x1, 0x1, 5);
	/* [7:7]: STRUP_AUXADC_RSTB_SEL; need to check with ABB */
	ret = pmic_config_interface(0x4E, 0x1, 0x1, 7);
	/* [0:0]: STRUP_PWROFF_SEQ_EN; Ricky */
	ret = pmic_config_interface(0x50, 0x1, 0x1, 0);
	/* [1:1]: STRUP_PWROFF_PREOFF_EN; Ricky */
	ret = pmic_config_interface(0x50, 0x1, 0x1, 1);
	/* [9:9]: SPK_THER_SHDN_L_EN; */
	ret = pmic_config_interface(0x52, 0x1, 0x1, 9);
	/* [0:0]: RG_SPK_INTG_RST_L; */
	ret = pmic_config_interface(0x56, 0x1, 0x1, 0);
	/* [11:8]: RG_SPKPGA_GAIN; */
	ret = pmic_config_interface(0x64, 0x1, 0xF, 8);
	/* [6:6]: RG_RTC_75K_CK_PDN; JT */
	ret = pmic_config_interface(0x102, 0x1, 0x1, 6);
	/* [11:11]: RG_DRV_32K_CK_PDN; JT */
	ret = pmic_config_interface(0x102, 0x0, 0x1, 11);
	/* [15:15]: RG_BUCK32K_PDN; JT */
	ret = pmic_config_interface(0x102, 0x1, 0x1, 15);
	/* [12:12]: RG_EFUSE_CK_PDN; JT */
	ret = pmic_config_interface(0x108, 0x1, 0x1, 12);
	/* [5:5]: RG_AUXADC_CTL_CK_PDN; JT */
	ret = pmic_config_interface(0x10E, 0x1, 0x1, 5);
	/* [4:4]: RG_SRCLKEN_HW_MODE; JT */
	ret = pmic_config_interface(0x120, 0x1, 0x1, 4);
	/* [5:5]: RG_OSC_HW_MODE; JT */
	ret = pmic_config_interface(0x120, 0x1, 0x1, 5);
	/* [15:8]: RG_TOP_CKTST2_RSV_15_8; JT */
	ret = pmic_config_interface(0x130, 0x1, 0xFF, 8);
	/* [0:0]: RG_SMT_SYSRSTB; Ricky */
	ret = pmic_config_interface(0x148, 0x1, 0x1, 0);
	/* [1:1]: RG_SMT_INT; Ricky */
	ret = pmic_config_interface(0x148, 0x1, 0x1, 1);
	/* [2:2]: RG_SMT_SRCLKEN; Ricky */
	ret = pmic_config_interface(0x148, 0x1, 0x1, 2);
	/* [3:3]: RG_SMT_RTC_32K1V8; Ricky */
	ret = pmic_config_interface(0x148, 0x1, 0x1, 3);
	/* [2:2]: RG_INT_EN_BAT_L; Ricky */
	ret = pmic_config_interface(0x160, 0x1, 0x1, 2);
	/* [6:6]: RG_INT_EN_THR_L; Ricky */
	ret = pmic_config_interface(0x160, 0x1, 0x1, 6);
	/* [7:7]: RG_INT_EN_THR_H; Ricky */
	ret = pmic_config_interface(0x160, 0x1, 0x1, 7);
	/* [1:1]: RG_INT_EN_FCHRKEY; Ricky */
	ret = pmic_config_interface(0x166, 0x1, 0x1, 1);
	/* [5:4]: QI_VPROC_VSLEEP; TH */
	ret = pmic_config_interface(0x212, 0x2, 0x3, 4);
	/* [1:1]: VPROC_VOSEL_CTRL; Seven,JT */
	ret = pmic_config_interface(0x216, 0x1, 0x1, 1);
	/* [6:0]: VPROC_SFCHG_FRATE; Seven,JT, the value should setting before EN */
	ret = pmic_config_interface(0x21C, 0x17, 0x7F, 0);
	/* [7:7]: VPROC_SFCHG_FEN; Seven,JT */
	ret = pmic_config_interface(0x21C, 0x1, 0x1, 7);
	/* [15:15]: VPROC_SFCHG_REN; Seven,JT */
	ret = pmic_config_interface(0x21C, 0x1, 0x1, 15);
	/* [6:0]: VPROC_VOSEL; for Low power, the value should after VPROC_VOSEL_CTRL */
	ret = pmic_config_interface(0x21E, 0x38, 0x7F, 0);
	/* [6:0]: VPROC_VOSEL_SLEEP; Seven, set value same to Vsleep */
	ret = pmic_config_interface(0x222, 0x18, 0x7F, 0);
	/* [1:0]: VPROC_TRANSTD; Seven,same with 6323 */
	ret = pmic_config_interface(0x230, 0x3, 0x3, 0);
	/* [5:4]: VPROC_VOSEL_TRANS_EN; Seven,need to recheck */
	ret = pmic_config_interface(0x230, 0x1, 0x3, 4);
	/* [8:8]: VPROC_VSLEEP_EN; Seven,JT */
	ret = pmic_config_interface(0x230, 0x1, 0x1, 8);
	/* [1:0]: RG_VSYS_SLP; Johnson,same 6323 */
	ret = pmic_config_interface(0x238, 0x3, 0x3, 0);
	/* [5:4]: QI_VSYS_VSLEEP; Johnson,re-confrim */
	ret = pmic_config_interface(0x238, 0x2, 0x3, 4);
	/* [6:0]: VSYS_SFCHG_FRATE; Johnson,same 6323 */
	ret = pmic_config_interface(0x242, 0x23, 0x7F, 0);
	/* [14:8]: VSYS_SFCHG_RRATE; Johnson,same 6323 */
	ret = pmic_config_interface(0x242, 0x11, 0x7F, 8);
	/* [15:15]: VSYS_SFCHG_REN; Johnson,same 6323 */
	ret = pmic_config_interface(0x242, 0x1, 0x1, 15);
	/* [1:0]: VSYS_TRANSTD; Johnson,same 6323 */
	ret = pmic_config_interface(0x256, 0x3, 0x3, 0);
	/* [5:4]: VSYS_VOSEL_TRANS_EN; Johnson,same 6323 */
	ret = pmic_config_interface(0x256, 0x1, 0x3, 4);
	/* [8:8]: VSYS_VSLEEP_EN; Johnson,JT */
	ret = pmic_config_interface(0x256, 0x1, 0x1, 8);
	/* [1:1]: VSYS_VOSEL_CTRL; after 0x0256,Johnson */
	ret = pmic_config_interface(0x23C, 0x1, 0x1, 1);
	/* [9:8]: RG_VPA_CSL; Johnson,same 6323 */
	ret = pmic_config_interface(0x302, 0x1, 0x3, 8);
	/* [15:14]: RG_VPA_ZX_OS; Johnson,same 6323 */
	ret = pmic_config_interface(0x302, 0x1, 0x3, 14);
	/* [7:7]: VPA_SFCHG_FEN; Johnson,JT */
	ret = pmic_config_interface(0x310, 0x1, 0x1, 7);
	/* [15:15]: VPA_SFCHG_REN; Johnson,JT */
	ret = pmic_config_interface(0x310, 0x1, 0x1, 15);
	/* [0:0]: VPA_DLC_MAP_EN; Johnson,same 6323 */
	ret = pmic_config_interface(0x326, 0x1, 0x1, 0);
	/* [0:0]: VTCXO_LP_SEL; TH,JT */
	ret = pmic_config_interface(0x402, 0x1, 0x1, 0);
	/* [10:10]: RG_VTCXO_EN; TH,JT */
	ret = pmic_config_interface(0x402, 0x1, 0x1, 10);
	/* [11:11]: VTCXO_ON_CTRL; TH,JT */
	ret = pmic_config_interface(0x402, 0x0, 0x1, 11);
	/* [0:0]: VA_LP_SEL; TH,JT */
	ret = pmic_config_interface(0x404, 0x1, 0x1, 0);
	/* [9:8]: RG_VA_SENSE_SEL; Check with ricky or KH */
	ret = pmic_config_interface(0x404, 0x2, 0x3, 8);
	/* [0:0]: VIO28_LP_SEL; JT,TH */
	ret = pmic_config_interface(0x500, 0x1, 0x1, 0);
	/* [0:0]: VUSB_LP_SEL; JT,TH */
	ret = pmic_config_interface(0x502, 0x1, 0x1, 0);
	/* [0:0]: VMC_LP_SEL; JT,TH */
	ret = pmic_config_interface(0x504, 0x1, 0x1, 0);
	/* [0:0]: VMCH_LP_SEL; JT,TH */
	ret = pmic_config_interface(0x506, 0x1, 0x1, 0);
	/* [0:0]: VEMC_3V3_LP_SEL; JT,TH */
	ret = pmic_config_interface(0x508, 0x1, 0x1, 0);
	/* [0:0]: RG_STB_SIM1_SIO; CC */
	ret = pmic_config_interface(0x514, 0x1, 0x1, 0);
	/* [0:0]: VSIM1_LP_SEL; Chek with Lower power team */
	ret = pmic_config_interface(0x516, 0x1, 0x1, 0);
	/* [0:0]: VSIM2_LP_SEL; Chek with Lower power team */
	ret = pmic_config_interface(0x518, 0x1, 0x1, 0);
	/* [0:0]: RG_STB_SIM2_SIO; CC */
	ret = pmic_config_interface(0x524, 0x1, 0x1, 0);
	/* [1:1]: VRF18_ON_CTRL; JT,TH */
	ret = pmic_config_interface(0x550, 0x1, 0x1, 1);
	/* [0:0]: VM_LP_SEL; JT,TH */
	ret = pmic_config_interface(0x552, 0x1, 0x1, 0);
	/* [0:0]: VIO18_LP_SEL; JT,TH */
	ret = pmic_config_interface(0x556, 0x1, 0x1, 0);
	/* [3:2]: RG_ADC_TRIM_CH6_SEL; Ricky */
	ret = pmic_config_interface(0x756, 0x1, 0x3, 2);
	/* [5:4]: RG_ADC_TRIM_CH5_SEL; Ricky */
	ret = pmic_config_interface(0x756, 0x1, 0x3, 4);
	/* [9:8]: RG_ADC_TRIM_CH3_SEL; Ricky */
	ret = pmic_config_interface(0x756, 0x1, 0x3, 8);
	/* [11:10]: RG_ADC_TRIM_CH2_SEL; Ricky */
	ret = pmic_config_interface(0x756, 0x1, 0x3, 10);
	/* [15:15]: RG_VREF18_ENB_MD; JT confirmed with ZF */
	ret = pmic_config_interface(0x778, 0x1, 0x1, 15);

	/* GPIO5 WORKAROUND */
	/* [7:7]: RG_VCN33_EN_BT */
	ret = pmic_config_interface(0x416, 0x1, 0x1, 7);
	/* [12:12]: RG_VCN33_EN_WIFI */
	ret = pmic_config_interface(0x418, 0x1, 0x1, 12);
	/* [1:1]: VCN33_LP_SET */
	ret = pmic_config_interface(0x420, 0x1, 0x1, 1);
	/* [14:14]: RG_VCN18_EN */
	ret = pmic_config_interface(0x512, 0x1, 0x1, 14);
	/* [1:1]: VCN18_LP_SET */
	ret = pmic_config_interface(0x512, 0x1, 0x1, 1);
	PMICLOG("[Kernel_PMIC_INIT_SETTING_V1] 2015-01-21...\n");

	/* External Buck WORKAROUND */
	/* [1:1]: STRUP_EXT_PMIC_SEL */
	ret = pmic_config_interface(0x04C, 0x1, 0x1, 1);

}

#if defined CONFIG_MTK_LEGACY
/*extern void pmu_drv_tool_customization_init(void);*/
#endif
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


/*
 * Dump all LDO status
 */
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
		PMICLOG("%s   status:%d     voltage:%duv    voltage_reg:%d\n",
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

		PMICLOG("%s   status:%d     voltage:%duv    voltage_reg:%d\n",
			mtk_ldos[i].desc.name, en, voltage, voltage_reg);
	}


	for (i = 0; i < ARRAY_SIZE(interrupts); i++) {
		for (j = 0; j < PMIC_INT_WIDTH; j++) {

			PMICLOG("[PMIC_INT][%s] interrupt issue times: %d\n",
				interrupts[i].interrupts[j].name,
				interrupts[i].interrupts[j].times);
		}
	}

	PMICLOG("Power Good Status=0x%x.\n", upmu_get_reg_value(MT6350_PGSTATUS));
	PMICLOG("OC Status0=0x%x.\n", upmu_get_reg_value(MT6350_OCSTATUS0));
	PMICLOG("OC Status1=0x%x.\n", upmu_get_reg_value(MT6350_OCSTATUS1));
	PMICLOG("Regulator Status0=0x%x.\n", upmu_get_reg_value(MT6350_EN_STATUS0));
	PMICLOG("Regulator Status1=0x%x.\n", upmu_get_reg_value(MT6350_EN_STATUS1));
	PMICLOG("Thermal Status=0x%x.\n", upmu_get_reg_value(MT6350_STRUP_CON5));
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

	for (i = 0; i < ARRAY_SIZE(interrupts); i++) {
		for (j = 0; j < PMIC_INT_WIDTH; j++) {

			seq_printf(m, "[PMIC_INT][%s] interrupt issue times: %d\n",
				   interrupts[i].interrupts[j].name,
				   interrupts[i].interrupts[j].times);
		}
	}
	seq_printf(m, "Power Good Status=0x%x.\n", upmu_get_reg_value(MT6350_PGSTATUS));
	seq_printf(m, "OC Status0=0x%x.\n", upmu_get_reg_value(MT6350_OCSTATUS0));
	seq_printf(m, "OC Status1=0x%x.\n", upmu_get_reg_value(MT6350_OCSTATUS1));
	seq_printf(m, "Regulator Status0=0x%x.\n", upmu_get_reg_value(MT6350_EN_STATUS0));
	seq_printf(m, "Regulator Status1=0x%x.\n", upmu_get_reg_value(MT6350_EN_STATUS1));
	seq_printf(m, "Thermal Status=0x%x.\n", upmu_get_reg_value(MT6350_STRUP_CON5));

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

		/* PMICLOG("=>charger_hv_detect_sw_workaround\n"); */
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
/*
 * low battery protect UT
 */
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
	/*char *pvalue = NULL; */
	unsigned int val = 0;

	PMICLOG("[store_low_battery_protect_ut]\n");

	if (buf != NULL && size != 0) {
		PMICLOG("[store_low_battery_protect_ut] buf is %s and size is %zu\n", buf, size);
		/*val = simple_strtoul(buf, &pvalue, 16); */
		ret = kstrtoul(buf, 16, (unsigned long *)&val);
		if (val <= 2) {
			PMICLOG("[store_low_battery_protect_ut] your input is %d\n", val);
			exec_low_battery_callback(val);
		} else {
			PMICLOG("[store_low_battery_protect_ut] wrong number (%d)\n", val);
		}
	}
	return size;
}

/* 664 */
static DEVICE_ATTR(low_battery_protect_ut, 0664, show_low_battery_protect_ut,
		   store_low_battery_protect_ut);

/*
 * low battery protect stop
 */
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
	/*char *pvalue = NULL; */
	unsigned int val = 0;

	PMICLOG("[store_low_battery_protect_stop]\n");

	if (buf != NULL && size != 0) {
		PMICLOG("[store_low_battery_protect_stop] buf is %s and size is %zu\n", buf, size);
		/* val = simple_strtoul(buf, &pvalue, 16); */
		ret = kstrtoul(buf, 16, (unsigned long *)&val);
		if ((val != 0) && (val != 1))
			val = 0;
		g_low_battery_stop = val;
		PMICLOG("[store_low_battery_protect_stop] g_low_battery_stop=%d\n",
			g_low_battery_stop);
	}
	return size;
}

/* 664 */
static DEVICE_ATTR(low_battery_protect_stop, 0664, show_low_battery_protect_stop, store_low_battery_protect_stop);

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

/* 664 */
static DEVICE_ATTR(low_battery_protect_level, 0664, show_low_battery_protect_level, store_low_battery_protect_level);
#endif

#ifdef BATTERY_OC_PROTECT
/*
 * battery OC protect UT
 */
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
	/*char *pvalue = NULL; */
	unsigned int val = 0;

	PMICLOG("[store_battery_oc_protect_ut]\n");

	if (buf != NULL && size != 0) {
		PMICLOG("[store_battery_oc_protect_ut] buf is %s and size is %zu\n", buf, size);
		/* val = simple_strtoul(buf, &pvalue, 16); */
		ret = kstrtoul(buf, 16, (unsigned long *)&val);
		if (val <= 1) {
			PMICLOG("[store_battery_oc_protect_ut] your input is %d\n", val);
			exec_battery_oc_callback(val);
		} else {
			PMICLOG("[store_battery_oc_protect_ut] wrong number (%d)\n", val);
		}
	}
	return size;
}

/* 664 */
static DEVICE_ATTR(battery_oc_protect_ut, 0664, show_battery_oc_protect_ut, store_battery_oc_protect_ut);

/*
 * battery OC protect stop
 */
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
	/*char *pvalue = NULL; */
	unsigned int val = 0;

	PMICLOG("[store_battery_oc_protect_stop]\n");

	if (buf != NULL && size != 0) {
		PMICLOG("[store_battery_oc_protect_stop] buf is %s and size is %zu\n", buf, size);
		/* val = simple_strtoul(buf, &pvalue, 16); */
		ret = kstrtoul(buf, 16, (unsigned long *)&val);
		if ((val != 0) && (val != 1))
			val = 0;
		g_battery_oc_stop = val;
		PMICLOG("[store_battery_oc_protect_stop] g_battery_oc_stop=%d\n",
			g_battery_oc_stop);
	}
	return size;
}

/*664 */
static DEVICE_ATTR(battery_oc_protect_stop, 0664, show_battery_oc_protect_stop, store_battery_oc_protect_stop);

/*
 * battery OC protect level
 */
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
/*664 */
static DEVICE_ATTR(battery_oc_protect_level, 0664, show_battery_oc_protect_level, store_battery_oc_protect_level);
#endif

#ifdef BATTERY_PERCENT_PROTECT
/*
 * battery percent protect UT
 */
static ssize_t show_battery_percent_protect_ut(struct device *dev, struct device_attribute *attr,
					       char *buf)
{
	PMICLOG("[show_battery_percent_protect_ut] g_battery_percent_level=%d\n",
		g_battery_percent_level);
	return sprintf(buf, "%u\n", g_battery_percent_level);
}

static ssize_t store_battery_percent_protect_ut(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t size)
{
	int ret = 0;
	/*char *pvalue = NULL; */
	unsigned int val = 0;

	PMICLOG("[store_battery_percent_protect_ut]\n");

	if (buf != NULL && size != 0) {
		PMICLOG("[store_battery_percent_protect_ut] buf is %s and size is %zu\n", buf,
			size);
		/* val = simple_strtoul(buf, &pvalue, 16); */
		ret = kstrtoul(buf, 16, (unsigned long *)&val);
		if (val <= 1) {
			PMICLOG("[store_battery_percent_protect_ut] your input is %d\n", val);
			exec_battery_percent_callback(val);
		} else {
			PMICLOG("[store_battery_percent_protect_ut] wrong number (%d)\n", val);
		}
	}
	return size;
}
/*664 */
static DEVICE_ATTR(battery_percent_protect_ut, 0664, show_battery_percent_protect_ut, store_battery_percent_protect_ut);

/*
 * battery percent protect stop
 */
static ssize_t show_battery_percent_protect_stop(struct device *dev, struct device_attribute *attr,
						 char *buf)
{
	PMICLOG("[show_battery_percent_protect_stop] g_battery_percent_stop=%d\n",
		g_battery_percent_stop);
	return sprintf(buf, "%u\n", g_battery_percent_stop);
}

static ssize_t store_battery_percent_protect_stop(struct device *dev, struct device_attribute *attr,
						  const char *buf, size_t size)
{
	int ret = 0;
	/*char *pvalue = NULL; */
	unsigned int val = 0;

	PMICLOG("[store_battery_percent_protect_stop]\n");

	if (buf != NULL && size != 0) {
		PMICLOG("[store_battery_percent_protect_stop] buf is %s and size is %zu\n", buf,
			size);
		/* val = simple_strtoul(buf, &pvalue, 16); */
		ret = kstrtoul(buf, 16, (unsigned long *)&val);
		if ((val != 0) && (val != 1))
			val = 0;
		g_battery_percent_stop = val;
		PMICLOG("[store_battery_percent_protect_stop] g_battery_percent_stop=%d\n",
			g_battery_percent_stop);
	}
	return size;
}

/*664 */
static DEVICE_ATTR(battery_percent_protect_stop, 0664, show_battery_percent_protect_stop,
	 store_battery_percent_protect_stop);

/*
 * battery percent protect level
 */
static ssize_t show_battery_percent_protect_level(struct device *dev, struct device_attribute *attr,
						  char *buf)
{
	PMICLOG("[show_battery_percent_protect_level] g_battery_percent_level=%d\n",
		g_battery_percent_level);
	return sprintf(buf, "%u\n", g_battery_percent_level);
}

static ssize_t store_battery_percent_protect_level(struct device *dev,
						   struct device_attribute *attr, const char *buf,
						   size_t size)
{
	PMICLOG("[store_battery_percent_protect_level] g_battery_percent_level=%d\n",
		g_battery_percent_level);

	return size;
}

/*664 */
static DEVICE_ATTR(battery_percent_protect_level, 0664,
	show_battery_percent_protect_level, store_battery_percent_protect_level);
#endif

#ifdef DLPT_FEATURE_SUPPORT
/*
 * DLPT UT
 */
static ssize_t show_dlpt_ut(struct device *dev, struct device_attribute *attr, char *buf)
{
	PMICLOG("[show_dlpt_ut] g_dlpt_val=%d\n", g_dlpt_val);
	return sprintf(buf, "%u\n", g_dlpt_val);
}

static ssize_t store_dlpt_ut(struct device *dev, struct device_attribute *attr, const char *buf,
			     size_t size)
{
	int ret = 0;
	/*char *pvalue = NULL; */
	unsigned int val = 0;

	PMICLOG("[store_dlpt_ut]\n");

	if (buf != NULL && size != 0) {
		PMICLOG("[store_dlpt_ut] buf is %s and size is %zu\n", buf, size);
		/* val = simple_strtoul(buf, &pvalue, 10); */
		ret = kstrtoul(buf, 10, (unsigned long *)&val);

		PMICLOG("[store_dlpt_ut] your input is %d\n", val);
		exec_dlpt_callback(val);
	}
	return size;
}

static DEVICE_ATTR(dlpt_ut, 0664, show_dlpt_ut, store_dlpt_ut);	/* 664 */

/*
 * DLPT stop
 */
static ssize_t show_dlpt_stop(struct device *dev, struct device_attribute *attr, char *buf)
{
	PMICLOG("[show_dlpt_stop] g_dlpt_stop=%d\n", g_dlpt_stop);
	return sprintf(buf, "%u\n", g_dlpt_stop);
}

static ssize_t store_dlpt_stop(struct device *dev, struct device_attribute *attr, const char *buf,
			       size_t size)
{
	int ret = 0;
	/*char *pvalue = NULL; */
	unsigned int val = 0;

	PMICLOG("[store_dlpt_stop]\n");

	if (buf != NULL && size != 0) {
		PMICLOG("[store_dlpt_stop] buf is %s and size is %zu\n", buf, size);
		/* val = simple_strtoul(buf, &pvalue, 16); */
		ret = kstrtoul(buf, 16, (unsigned long *)&val);
		if ((val != 0) && (val != 1))
			val = 0;
		g_dlpt_stop = val;
		PMICLOG("[store_dlpt_stop] g_dlpt_stop=%d\n", g_dlpt_stop);
	}
	return size;
}

static DEVICE_ATTR(dlpt_stop, 0664, show_dlpt_stop, store_dlpt_stop);	/* 664 */

/*
 * DLPT level
 */
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

static DEVICE_ATTR(dlpt_level, 0664, show_dlpt_level, store_dlpt_level);	/* 664 */
#endif

/*
 * system function
 */
#if 0
/*static unsigned long pmic_node = 0;*/
static unsigned long pmic_node;

static int fb_early_init_dt_get_chosen(unsigned long node, const char *uname, int depth, void *data)
{
	if (depth != 1 || (strcmp(uname, "chosen") != 0 && strcmp(uname, "chosen@0") != 0))
		return 0;
	pmic_node = node;
	return 1;
}
#endif

int g_plug_out_status = 0;

static int detect_battery_plug_out_status(void)
{
	signed int plug_out_1, plug_out_2;

	plug_out_1 = pmic_get_register_value(PMIC_STRUP_PWROFF_PREOFF_EN);
	plug_out_2 = pmic_get_register_value(PMIC_STRUP_PWROFF_SEQ_EN);

	PMICLOG("plug_out_1=%d, plug_out_2=%d\n", plug_out_1, plug_out_2);
	if (plug_out_1 == 0 && plug_out_2 == 0)
		g_plug_out_status = 1;
	else
		g_plug_out_status = 0;
	return g_plug_out_status;
}

int get_battery_plug_out_status(void)
{
	return g_plug_out_status;
}

static void pmic_early_suspend(void)
{
	pmic_set_register_value(PMIC_RG_VREF18_ENB, 0);
	pmic_set_register_value(PMIC_RG_CLKSQ_EN_AUX, 1);
	pmic_set_register_value(PMIC_RG_AUD26M_DIV4_CK_PDN, 0);
	pmic_set_register_value(PMIC_RG_AUXADC_SDM_SEL_HW_MODE, 0);
	pmic_set_register_value(PMIC_RG_AUXADC_SDM_CK_HW_MODE, 1);
	pmic_set_register_value(PMIC_RG_AUXADC_SDM_CK_SEL, 0);
	pmic_set_register_value(PMIC_RG_AUXADC_SDM_CK_PDN, 0);
	pmic_set_register_value(PMIC_RG_AUXADC_SDM_CK_WAKE_PDN, 1);
}

static void pmic_late_resume(void)
{
	pmic_set_register_value(PMIC_RG_VREF18_ENB, 0);
	pmic_set_register_value(PMIC_RG_CLKSQ_EN_AUX, 1);
	pmic_set_register_value(PMIC_RG_AUD26M_DIV4_CK_PDN, 0);
	pmic_set_register_value(PMIC_RG_AUXADC_SDM_SEL_HW_MODE, 0);
	pmic_set_register_value(PMIC_RG_AUXADC_SDM_CK_HW_MODE, 0);
	pmic_set_register_value(PMIC_RG_AUXADC_SDM_CK_SEL, 0);
	pmic_set_register_value(PMIC_RG_AUXADC_SDM_CK_PDN, 0);
	pmic_set_register_value(PMIC_RG_AUXADC_SDM_CK_WAKE_PDN, 0);
}

static int pmic_fb_notifier_callback(struct notifier_block *self, unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int blank;

	/* skip if it's not a blank event */
	if (event != FB_EVENT_BLANK)
		return 0;

	if (evdata == NULL)
		return 0;
	if (evdata->data == NULL)
		return 0;

	blank = *(int *)evdata->data;

	switch (blank) {
	/* LCM ON */
	case FB_BLANK_UNBLANK:
		pmic_late_resume();
		break;
	/* LCM OFF */
	case FB_BLANK_POWERDOWN:
		pmic_early_suspend();
		break;
	default:
		break;
	}

	return 0;
}

static struct notifier_block pmic_fb_notifier = {
	.notifier_call = pmic_fb_notifier_callback,
};

static int pmic_mt_probe(struct platform_device *dev)
{
	int ret_device_file = 0, i;

	PMICLOG_DBG("******** MT pmic driver probe!! ********\n");

	detect_battery_plug_out_status();
	dump_ldo_status_read_debug();

	/* get PMIC CID */
	PMICLOG("PMIC CID=0x%x.\n", pmic_get_register_value(PMIC_CID));

	PMICLOG("Power Good Status=0x%x.\n", upmu_get_reg_value(MT6350_PGSTATUS));
	PMICLOG("OC Status0=0x%x.\n", upmu_get_reg_value(MT6350_OCSTATUS0));
	PMICLOG("OC Status1=0x%x.\n", upmu_get_reg_value(MT6350_OCSTATUS1));
	PMICLOG("Regulator Status0=0x%x.\n", upmu_get_reg_value(MT6350_EN_STATUS0));
	PMICLOG("Regulator Status1=0x%x.\n", upmu_get_reg_value(MT6350_EN_STATUS1));
	PMICLOG("Thermal Status=0x%x.\n", upmu_get_reg_value(MT6350_STRUP_CON5));
	/* pmic initial setting */
	PMIC_INIT_SETTING_V1();
	PMICLOG("[PMIC_INIT_SETTING_V1] Done\n");
#if !defined CONFIG_MTK_LEGACY
/*      replace by DTS*/
#else
	PMIC_CUSTOM_SETTING_V1();
	PMICLOG("[PMIC_CUSTOM_SETTING_V1] Done\n");
#endif				/*End of #if !defined CONFIG_MTK_LEGACY */

/* #if defined(CONFIG_MTK_FPGA) */
#if 0
	PMICLOG("[PMIC_EINT_SETTING] disable when CONFIG_MTK_FPGA\n");
#else
	/* PMIC Interrupt Service */
	pmic_thread_handle = kthread_create(pmic_thread_kthread, (void *)NULL, "pmic_thread");
	if (IS_ERR(pmic_thread_handle)) {
		pmic_thread_handle = NULL;
		PMICLOG("[pmic_thread_kthread_mt6350] creation fails\n");
	} else {
		wake_up_process(pmic_thread_handle);
		PMICLOG_DBG("[pmic_thread_kthread_mt6350] kthread_create Done\n");
	}
	PMIC_EINT_SETTING();
	PMICLOG("[PMIC_EINT_SETTING] Done\n");
#endif


	mtk_regulator_init(dev);

#ifdef LOW_BATTERY_PROTECT
	low_battery_protect_init();
#else
	PMICLOG("[PMIC] no define LOW_BATTERY_PROTECT\n");
#endif

#ifdef BATTERY_OC_PROTECT
	battery_oc_protect_init();
#else
	PMICLOG("[PMIC] no define BATTERY_OC_PROTECT\n");
#endif

#ifdef BATTERY_PERCENT_PROTECT
	bat_percent_notify_init();
#else
	PMICLOG("[PMIC] no define BATTERY_PERCENT_PROTECT\n");
#endif

#ifdef DLPT_FEATURE_SUPPORT
	dlpt_notify_init();
#else
	PMICLOG("[PMIC] no define DLPT_FEATURE_SUPPORT\n");
#endif

#if 0
	pmic_set_register_value(PMIC_AUXADC_CK_AON, 1);
	pmic_set_register_value(PMIC_RG_CLKSQ_EN_AUX_AP_MODE, 1);
	PMICLOG("[PMIC] auxadc 26M test : Reg[0x%x]=0x%x, Reg[0x%x]=0x%x\n",
		MT6350_AUXADC_CON0, upmu_get_reg_value(MT6350_AUXADC_CON0),
		MT6350_TOP_CLKSQ, upmu_get_reg_value(MT6350_TOP_CLKSQ)
	    );
#endif

	pmic_debug_init();
	PMICLOG("[PMIC] pmic_debug_init : done.\n");

	pmic_ftm_init();



	/* EM BUCK voltage & Status */
	for (i = 0; i < ARRAY_SIZE(mtk_bucks); i++) {
		/* PMICLOG("[PMIC] register buck id=%d\n",i); */
		ret_device_file = device_create_file(&(dev->dev), &mtk_bucks[i].en_att);
		ret_device_file = device_create_file(&(dev->dev), &mtk_bucks[i].voltage_att);
	}

	/* EM ldo voltage & Status */
	for (i = 0; i < ARRAY_SIZE(mtk_ldos); i++) {
		/* PMICLOG("[PMIC] register ldo id=%d\n",i); */
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

	PMICLOG("[PMIC] device_create_file for EM : done.\n");

	/* pwrkey_sw_workaround_init(); */
	if (fb_register_client(&pmic_fb_notifier)) {
		PMICLOG("register FB client failed!\n");
		return 0;
	}
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
}

static int pmic_mt_suspend(struct platform_device *dev, pm_message_t state)
{
	PMICLOG_DBG("******** MT pmic driver suspend!! ********\n");

#ifdef LOW_BATTERY_PROTECT
	lbat_min_en_setting(0);
	lbat_max_en_setting(0);

	PMICLOG("Reg[0x%x]=0x%x, Reg[0x%x]=0x%x, Reg[0x%x]=0x%x\n",
		MT6350_AUXADC_LBAT3, upmu_get_reg_value(MT6350_AUXADC_LBAT3),
		MT6350_AUXADC_LBAT4, upmu_get_reg_value(MT6350_AUXADC_LBAT4),
		MT6350_INT_CON0, upmu_get_reg_value(MT6350_INT_CON0)
	    );
#endif

#ifdef BATTERY_OC_PROTECT
	bat_oc_h_en_setting(0);
	bat_oc_l_en_setting(0);

	PMICLOG("Reg[0x%x]=0x%x, Reg[0x%x]=0x%x, Reg[0x%x]=0x%x\n",
		MT6350_FGADC_CON23, upmu_get_reg_value(MT6350_FGADC_CON23),
		MT6350_FGADC_CON24, upmu_get_reg_value(MT6350_FGADC_CON24),
		MT6350_INT_CON2, upmu_get_reg_value(MT6350_INT_CON2)
	    );
#endif
#if 1
	/*upmu_set_rg_vref18_enb(1);*/
	pmic_set_register_value(PMIC_RG_VREF18_ENB, 1);
	/*upmu_set_rg_adc_deci_gdly(1);*/
	pmic_set_register_value(PMIC_RG_ADC_DECI_GDLY, 1);
	/*upmu_set_rg_clksq_en_aux(1);*/
	pmic_set_register_value(PMIC_RG_CLKSQ_EN_AUX, 1);
	/*upmu_set_rg_aud26m_div4_ck_pdn(0);*/
	pmic_set_register_value(PMIC_RG_AUD26M_DIV4_CK_PDN, 0);
	/*upmu_set_rg_auxadc_sdm_sel_hw_mode(1);*/
	pmic_set_register_value(PMIC_RG_AUXADC_SDM_SEL_HW_MODE, 1);
	/*upmu_set_rg_auxadc_sdm_ck_hw_mode(1);*/
	pmic_set_register_value(PMIC_RG_AUXADC_SDM_CK_HW_MODE, 1);
	/*upmu_set_rg_auxadc_sdm_ck_sel(0);*/
	pmic_set_register_value(PMIC_RG_AUXADC_SDM_CK_SEL, 0);
	/*upmu_set_rg_auxadc_sdm_ck_pdn(0);*/
	pmic_set_register_value(PMIC_RG_AUXADC_SDM_CK_PDN, 0);
	/*upmu_set_rg_auxadc_sdm_ck_wake_pdn(0);*/
	pmic_set_register_value(PMIC_RG_AUXADC_SDM_CK_WAKE_PDN, 0);
	PMICLOG("MT pmic driver suspend henry %x %x %x!!\n",
		pmic_get_register_value(PMIC_RG_VREF18_ENB),
		pmic_get_register_value(PMIC_RG_AUXADC_SDM_SEL_HW_MODE),
		pmic_get_register_value(PMIC_RG_AUXADC_SDM_CK_WAKE_PDN));
#endif
	return 0;
}

static int pmic_mt_resume(struct platform_device *dev)
{
	PMICLOG_DBG("******** MT pmic driver resume!! ********\n");

#ifdef LOW_BATTERY_PROTECT
	lbat_min_en_setting(0);
	lbat_max_en_setting(0);
	mdelay(1);

	if (g_low_battery_level == 1) {
		lbat_min_en_setting(1);
		lbat_max_en_setting(1);
	} else if (g_low_battery_level == 2) {
		/* lbat_min_en_setting(0); */
		lbat_max_en_setting(1);
	} else {		/* 0 */

		lbat_min_en_setting(1);
		/* lbat_max_en_setting(0); */
	}

	PMICLOG("Reg[0x%x]=0x%x, Reg[0x%x]=0x%x, Reg[0x%x]=0x%x\n",
		MT6350_AUXADC_LBAT3, upmu_get_reg_value(MT6350_AUXADC_LBAT3),
		MT6350_AUXADC_LBAT4, upmu_get_reg_value(MT6350_AUXADC_LBAT4),
		MT6350_INT_CON0, upmu_get_reg_value(MT6350_INT_CON0)
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
		MT6350_FGADC_CON23, upmu_get_reg_value(MT6350_FGADC_CON23),
		MT6350_FGADC_CON24, upmu_get_reg_value(MT6350_FGADC_CON24),
		MT6350_INT_CON2, upmu_get_reg_value(MT6350_INT_CON2)
	    );
#endif
#if 1
	/*upmu_set_rg_vref18_enb(0);*/
	pmic_set_register_value(PMIC_RG_VREF18_ENB, 0);
	/*upmu_set_rg_clksq_en_aux(1);*/
	pmic_set_register_value(PMIC_RG_CLKSQ_EN_AUX, 1);
	/*upmu_set_rg_aud26m_div4_ck_pdn(0);*/
	pmic_set_register_value(PMIC_RG_AUD26M_DIV4_CK_PDN, 0);
	/*upmu_set_rg_auxadc_sdm_sel_hw_mode(0);*/
	pmic_set_register_value(PMIC_RG_AUXADC_SDM_SEL_HW_MODE, 0);
	/*upmu_set_rg_auxadc_sdm_ck_hw_mode(1);*/
	pmic_set_register_value(PMIC_RG_AUXADC_SDM_CK_HW_MODE, 1);
	/*upmu_set_rg_auxadc_sdm_ck_sel(0);*/
	pmic_set_register_value(PMIC_RG_AUXADC_SDM_CK_SEL, 0);
	/*upmu_set_rg_auxadc_sdm_ck_pdn(0);*/
	pmic_set_register_value(PMIC_RG_AUXADC_SDM_CK_PDN, 0);
	/*upmu_set_rg_auxadc_sdm_ck_wake_pdn(1);*/
	pmic_set_register_value(PMIC_RG_AUXADC_SDM_CK_WAKE_PDN, 1);
#endif
	PMICLOG("MT pmic driver resume henry %x %x %x!!\n",
		pmic_get_register_value(PMIC_RG_VREF18_ENB),
		pmic_get_register_value(PMIC_RG_AUXADC_SDM_SEL_HW_MODE),
		pmic_get_register_value(PMIC_RG_AUXADC_SDM_CK_WAKE_PDN));
	return 0;
}

struct platform_device pmic_mt_device = {
	.name = "mt-pmic",
	.id = -1,
};

static struct platform_driver pmic_mt_driver = {
	.probe = pmic_mt_probe,
	.remove = pmic_mt_remove,
	.shutdown = pmic_mt_shutdown,
	/* #ifdef CONFIG_PM */
	.suspend = pmic_mt_suspend,
	.resume = pmic_mt_resume,
	/* #endif */
	.driver = {
		   .name = "mt-pmic",
		   },
};

/*
 * PMIC mudule init/exit
 */
static int __init pmic_mt_init(void)
{
	int ret;

#ifdef CONFIG_PM_WAKELOCKS
	wakeup_source_init(&pmicThread_lock, "pmicThread_lock_mt6350 wakelock");
#ifdef BATTERY_PERCENT_PROTECT
	wakeup_source_init(&bat_percent_notify_lock, "bat_percent_notify_lock wakelock");
#endif

#else
	wake_lock_init(&pmicThread_lock, WAKE_LOCK_SUSPEND, "pmicThread_lock_mt6350 wakelock");
#ifdef BATTERY_PERCENT_PROTECT
	wake_lock_init(&bat_percent_notify_lock, WAKE_LOCK_SUSPEND,
		       "bat_percent_notify_lock wakelock");
#endif

#endif				/* #ifdef CONFIG_PM_WAKELOCKS */

#ifdef DLPT_FEATURE_SUPPORT
#ifdef CONFIG_PM_WAKELOCKS
	wakeup_source_init(&dlpt_notify_lock, "dlpt_notify_lock wakelock");
#else
	wake_lock_init(&dlpt_notify_lock, WAKE_LOCK_SUSPEND, "dlpt_notify_lock wakelock");
#endif
#endif				/* #ifdef DLPT_FEATURE_SUPPORT */

#if !defined CONFIG_MTK_LEGACY
#ifdef CONFIG_OF
	PMICLOG("pmic_regulator_init_OF\n");

	/* PMIC device driver register */
	ret = platform_device_register(&pmic_mt_device);
	if (ret) {
		PMICLOG("****[pmic_mt_init] Unable to device register(%d)\n", ret);
		return ret;
	}
	ret = platform_driver_register(&pmic_mt_driver);
	if (ret) {
		PMICLOG("****[pmic_mt_init] Unable to register driver (%d)\n", ret);
		return ret;
	}
	ret = platform_driver_register(&mt_pmic_driver);
	if (ret) {
		PMICLOG("****[pmic_mt_init] Unable to register driver by DT(%d)\n", ret);
		return ret;
	}
#endif				/* End of #ifdef CONFIG_OF */
#else
	PMICLOG("pmic_regulator_init\n");
	/* PMIC device driver register */
	ret = platform_device_register(&pmic_mt_device);
	if (ret) {
		PMICLOG("****[pmic_mt_init] Unable to device register(%d)\n", ret);
		return ret;
	}
	ret = platform_driver_register(&pmic_mt_driver);
	if (ret) {
		PMICLOG("****[pmic_mt_init] Unable to register driver (%d)\n", ret);
		return ret;
	}
#endif				/* End of #if !defined CONFIG_MTK_LEGACY */

#if 0				/* #ifdef CONFIG_HAS_EARLYSUSPEND */
	/* register_early_suspend(&pmic_early_suspend_desc); */
#endif

	pmic_auxadc_init();

	PMICLOG("****[pmic_mt_init] Initialization : DONE !!\n");

	return 0;
}

static void __exit pmic_mt_exit(void)
{
#if !defined CONFIG_MTK_LEGACY
#ifdef CONFIG_OF
	platform_driver_unregister(&mt_pmic_driver);
#endif
#endif				/* End of #if !defined CONFIG_MTK_LEGACY */
}
fs_initcall(pmic_mt_init);

/* module_init(pmic_mt_init); */
module_exit(pmic_mt_exit);

MODULE_AUTHOR("Anderson Tsai");
MODULE_DESCRIPTION("MT PMIC Device Driver");
MODULE_LICENSE("GPL");

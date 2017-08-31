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
#include "include/pmic_regulator.h"
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
/*#include "include/extbuck/fan53526.h" TBD*/
#endif

/*****************************************************************************
 * PMIC related define
 ******************************************************************************/
static DEFINE_MUTEX(pmic_lock_mutex);
#define PMIC_EINT_SERVICE

/*****************************************************************************
 * PMIC read/write APIs
 ******************************************************************************/
#if 0				/*defined(CONFIG_FPGA_EARLY_PORTING)*/
    /* no CONFIG_PMIC_HW_ACCESS_EN */
#else
#define CONFIG_PMIC_HW_ACCESS_EN
#endif


/*---IPI Mailbox define---*/
/*#define IPIMB*/
#if defined(IPIMB)
#include <mach/mtk_pmic_ipi.h>
#endif
static DEFINE_MUTEX(pmic_access_mutex);
/*--- Global suspend state ---*/
static bool pmic_suspend_state;

/* For whitney only(set boot voltage in preloader) */
#define MT6799_BOOT_VOL 1

static unsigned int vmd1_vosel = 0x40;
static unsigned int vmodem_vosel = 0x48;
static unsigned int vsram_vmd_vosel = 0x58;

void vmd1_pmic_setting_on(void)
{
	unsigned int vmd1_en = 0;
	unsigned int vmodem_en = 0;
	unsigned int vsram_vmd_en = 0;

	/*---VMD1, VMODEM, VSRAM_VMD Voltage Select---*/
	/*--VMD1 0.8V: 0x40 (0x40*0.00625+0.4 =0.8V)--*/
	pmic_set_register_value(PMIC_RG_BUCK_VMD1_VOSEL, vmd1_vosel);
	/*--0x10 (0x10*0.00625+0.4 =0.5V) SLEEP_VOLTAGE & VOSEL_SLEEP need the same --*/
	pmic_set_register_value(PMIC_RG_BUCK_VMD1_VOSEL_SLEEP, 0x10);
	/*--VMODEM 0.85V: 0x48 (0x48*0.00625+0.4 =0.85V)--*/
	pmic_set_register_value(PMIC_RG_BUCK_VMODEM_VOSEL, vmodem_vosel);
	/*--0x10 (0x10*0.00625+0.4 =0.5V) SLEEP_VOLTAGE & VOSEL_SLEEP need the same --*/
	pmic_set_register_value(PMIC_RG_BUCK_VMODEM_VOSEL_SLEEP, 0x10);
	/*--VSRAM_VMD 0.95V: 0x58 (0x58*0.00625+0.4 =0.95V)--*/
	pmic_set_register_value(PMIC_RG_VSRAM_VMD_VOSEL, vsram_vmd_vosel);
	/*--0x10 (0x10*0.00625+0.4 =0.5V) SLEEP_VOLTAGE & VOSEL_SLEEP need the same --*/
	pmic_set_register_value(PMIC_RG_VSRAM_VMD_VOSEL_SLEEP, 0x10);

	/* Enable FPFM before enable BUCK, SW workaround to avoid VMD1/VMODEM overshoot */
	pmic_config_interface(0x0F9C, 0x1, 0x1, 12);	/* 0x0F9C[12] = 1 */
	pmic_config_interface(0x0F88, 0x1, 0x1, 12);	/* 0x0F88[12] = 1 */

	/*---VMD1, VMODEM, VSRAM_VMD ENABLE---*/
	pmic_set_register_value(PMIC_RG_BUCK_VMD1_EN, 1);
	pmic_set_register_value(PMIC_RG_BUCK_VMODEM_EN, 1);
	pmic_set_register_value(PMIC_RG_VSRAM_VMD_SW_EN, 1);
	udelay(220);

	/* Disable FPFM after enable BUCK, SW workaround to avoid VMD1/VMODEM overshoot */
	pmic_config_interface(0x0F9C, 0x0, 0x1, 12);	/* 0x0F9C[12] = 0 */
	pmic_config_interface(0x0F88, 0x0, 0x1, 12);	/* 0x0F88[12] = 0 */

	vmd1_en = pmic_get_register_value(PMIC_DA_QI_VMD1_EN);
	vmodem_en = pmic_get_register_value(PMIC_DA_QI_VMODEM_EN);
	vsram_vmd_en = pmic_get_register_value(PMIC_DA_QI_VSRAM_VMD_EN);

	if (!vmd1_en || !vmodem_en || !vsram_vmd_en)
		pr_err("[vmd1_pmic_setting_on] VMD1 = %d, VMODEM = %d, VSRAM_VMD = %d\n",
			vmd1_en, vmodem_en, vsram_vmd_en);
}

void vmd1_pmic_setting_off(void)
{
	unsigned int vmd1_en = 0;
	unsigned int vmodem_en = 0;
	unsigned int vsram_vmd_en = 0;

	/*---VMD1, VMODEM, VSRAM_VMD DISABLE---*/
	pmic_set_register_value(PMIC_RG_BUCK_VMD1_EN, 0);
	pmic_set_register_value(PMIC_RG_BUCK_VMODEM_EN, 0);
	pmic_set_register_value(PMIC_RG_VSRAM_VMD_SW_EN, 0);
	udelay(220);

	vmd1_en = pmic_get_register_value(PMIC_DA_QI_VMD1_EN);
	vmodem_en = pmic_get_register_value(PMIC_DA_QI_VMODEM_EN);
	vsram_vmd_en = pmic_get_register_value(PMIC_DA_QI_VSRAM_VMD_EN);

	if (vmd1_en || vmodem_en || vsram_vmd_en)
		pr_err("[vmd1_pmic_setting_off] VMD1 = %d, VMODEM = %d, VSRAM_VMD = %d\n",
			vmd1_en, vmodem_en, vsram_vmd_en);
}

unsigned int pmic_read_interface(unsigned int RegNum, unsigned int *val, unsigned int MASK, unsigned int SHIFT)
{
	unsigned int return_value = 0;

#if defined(CONFIG_PMIC_HW_ACCESS_EN)
	unsigned int pmic_reg = 0;
	unsigned int rdata;

	return_value = pwrap_read((RegNum), &rdata);
	pmic_reg = rdata;
	if (return_value != 0) {
		pr_err("[pmic_read_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
		mutex_unlock(&pmic_access_mutex);
		return return_value;
	}
	/*PMICLOG"[pmic_read_interface] Reg[%x]=0x%x\n", RegNum, pmic_reg);*/

	pmic_reg &= (MASK << SHIFT);
	*val = (pmic_reg >> SHIFT);
	/*PMICLOG"[pmic_read_interface] val=0x%x\n", *val);*/

#else
	/*PMICLOG("[pmic_read_interface] Can not access HW PMIC\n");*/
#endif	/*defined(CONFIG_PMIC_HW_ACCESS_EN)*/

	return return_value;
}

unsigned int pmic_config_interface(unsigned int RegNum, unsigned int val, unsigned int MASK, unsigned int SHIFT)
{
	unsigned int return_value = 0;

#if defined(CONFIG_PMIC_HW_ACCESS_EN)
#ifndef IPIMB
	unsigned int pmic_reg = 0;
	unsigned int rdata;
#endif
	/*if ((pmic_suspend_state == true) && irqs_disabled())*/
	if (preempt_count() > 0 || irqs_disabled() || system_state != SYSTEM_RUNNING || oops_in_progress)
		return pmic_config_interface_nolock(RegNum, val, MASK, SHIFT);
#ifndef IPIMB
	mutex_lock(&pmic_access_mutex);

	return_value = pwrap_read((RegNum), &rdata);
	pmic_reg = rdata;
	if (return_value != 0) {
		pr_err("[pmic_config_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
		mutex_unlock(&pmic_access_mutex);
		return return_value;
	}
	/*PMICLOG"[pmic_config_interface] Reg[%x]=0x%x\n", RegNum, pmic_reg);*/

	pmic_reg &= ~(MASK << SHIFT);
	pmic_reg |= (val << SHIFT);

	return_value = pwrap_write((RegNum), pmic_reg);
	if (return_value != 0) {
		pr_err("[pmic_config_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
		mutex_unlock(&pmic_access_mutex);
		return return_value;
	}
	/*PMICLOG"[pmic_config_interface] write Reg[%x]=0x%x\n", RegNum, pmic_reg);*/

	mutex_unlock(&pmic_access_mutex);

#else /*---IPIMB---*/
	mutex_lock(&pmic_access_mutex);

	return_value = pmic_ipi_config_interface(RegNum, val, MASK, SHIFT, 1);

	if (return_value)
		PMICLOG("IPIMB write data fail\n");
	else
		PMICLOG("IPIMB write data =(%x,%x,%x,%x)\n", RegNum, val, MASK, SHIFT);

	mutex_unlock(&pmic_access_mutex);
#endif /*---!IPIMB---*/

#else
	/*PMICLOG("[pmic_config_interface] Can not access HW PMIC\n");*/
#endif	/*defined(CONFIG_PMIC_HW_ACCESS_EN)*/

	return return_value;
}

unsigned int pmic_read_interface_nolock(unsigned int RegNum, unsigned int *val, unsigned int MASK, unsigned int SHIFT)
{
	return pmic_read_interface(RegNum, val, MASK, SHIFT);
}

unsigned int pmic_config_interface_nolock(unsigned int RegNum, unsigned int val, unsigned int MASK, unsigned int SHIFT)
{
	unsigned int return_value = 0;

#if defined(CONFIG_PMIC_HW_ACCESS_EN)
#ifndef IPIMB
	unsigned int pmic_reg = 0;
	unsigned int rdata;

	/* pmic wrapper has spinlock protection. pmic do not to do it again */

	return_value = pwrap_read((RegNum), &rdata);
	pmic_reg = rdata;
	if (return_value != 0) {
		pr_err("[pmic_config_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
		return return_value;
	}
	/*PMICLOG"[pmic_config_interface] Reg[%x]=0x%x\n", RegNum, pmic_reg); */

	pmic_reg &= ~(MASK << SHIFT);
	pmic_reg |= (val << SHIFT);

	return_value = pwrap_write((RegNum), pmic_reg);
	if (return_value != 0) {
		pr_err("[pmic_config_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
		return return_value;
	}
	/*PMICLOG"[pmic_config_interface] write Reg[%x]=0x%x\n", RegNum, pmic_reg); */

#else /*---IPIMB---*/
	return_value = pmic_ipi_config_interface(RegNum, val, MASK, SHIFT, 0);

	if (return_value)
		PMICLOG("IPIMB write data nolock fail\n");
	else
		PMICLOG("IPIMB write data nolock = (%x,%x,%x,%x)\n", RegNum, val, MASK, SHIFT);

#endif /*---IPIMB---*/




#else
	/*PMICLOG("[pmic_config_interface] Can not access HW PMIC\n"); */
#endif	/*defined(CONFIG_PMIC_HW_ACCESS_EN)*/

	return return_value;
}

unsigned int upmu_get_reg_value(unsigned int reg)
{
	unsigned int reg_val = 0;
	unsigned int ret = 0;

	ret = pmic_read_interface(reg, &reg_val, 0xFFFF, 0x0);
	return reg_val;
}
EXPORT_SYMBOL(upmu_get_reg_value);

void upmu_set_reg_value(unsigned int reg, unsigned int reg_val)
{
	unsigned int ret = 0;

	ret = pmic_config_interface(reg, reg_val, 0xFFFF, 0x0);
}

/* [Export API] */

/* SCP set VCORE voltage, return 0 if success, otherwise return set voltage(uV) */
unsigned int pmic_scp_set_vcore(unsigned int voltage)
{
	const char *name = "SSHUB_VCORE";
	unsigned int max_uV = 1200000;
	unsigned int min_uV = 400000;
	unsigned int uV_step = 6250;
	unsigned short value = 0;
	unsigned short read_val = 0;

	if (voltage > max_uV || voltage < min_uV) {
		pr_err("[PMIC]Set Wrong buck voltage for %s, range (%duV - %duV)\n",
			name, min_uV, max_uV);
		return voltage;
	}
	value = (voltage - min_uV) / uV_step;
	PMICLOG("%s Expected volt step: 0x%x\n", name, value);

	/*---Make sure BUCK VCORE ON before setting---*/
	if (pmic_get_register_value(PMIC_DA_QI_VCORE_EN)) {
		pmic_set_register_value(PMIC_RG_BUCK_VCORE_SSHUB_VOSEL, value);
		udelay(220);
		read_val = pmic_get_register_value(PMIC_RG_BUCK_VCORE_SSHUB_VOSEL);
		if (read_val == value)
			PMICLOG("Set %s Voltage to %duV pass\n", name, voltage);
		else {
			pr_err("[PMIC] Set %s Voltage fail with step = %d, read voltage = %duV\n",
				name, value, (read_val * uV_step + min_uV));
			return voltage;
		}
	} else {
		pr_err("[PMIC] Set %s Votage to %duV fail, due to buck non-enable\n", name, voltage);
		return voltage;
	}

	return 0;
}

/* SCP set VSRAM_VCORE voltage, return 0 if success, otherwise return set voltage(uV) */
unsigned int pmic_scp_set_vsram_vcore(unsigned int voltage)
{
	const char *name = "VSRAM_VCORE_SSHUB";
	unsigned int max_uV = 1200000;
	unsigned int min_uV = 400000;
	unsigned int uV_step = 6250;
	unsigned short value = 0;
	unsigned short read_val = 0;

	if (voltage > max_uV || voltage < min_uV) {
		pr_err("[PMIC]Set Wrong buck voltage for %s, range (%duV - %duV)\n",
			name, min_uV, max_uV);
		return voltage;
	}
	value = (voltage - min_uV) / uV_step;
	PMICLOG("%s Expected volt step: 0x%x\n", name, value);

	/*---Make sure BUCK VSRAM_VCORE ON before setting---*/
	if (pmic_get_register_value(PMIC_DA_QI_VSRAM_VCORE_EN)) {
		pmic_set_register_value(PMIC_RG_VSRAM_VCORE_SSHUB_VOSEL, value);
		udelay(220);
		read_val = pmic_get_register_value(PMIC_RG_VSRAM_VCORE_SSHUB_VOSEL);
		if (read_val == value)
			PMICLOG("Set %s Voltage to %duV pass\n", name, voltage);
		else {
			pr_err("[PMIC] Set %s Voltage fail with step = %d, read voltage = %duV\n",
				name, value, (read_val * uV_step + min_uV));
			return voltage;
		}
	} else {
		pr_err("[PMIC] Set %s Votage to %duV fail, due to buck non-enable\n", name, voltage);
		return voltage;
	}

	return 0;
}

/* enable/disable VSRAM_VCORE HW tracking, return 0 if success */
unsigned int enable_vsram_vcore_hw_tracking(unsigned int en)
{
	if (en != 1 && en != 0)
		return en;

	pmic_set_register_value(PMIC_RG_VSRAM_VCORE_TRACK_VBUCK_ON_CTRL, en);
	pmic_set_register_value(PMIC_RG_VSRAM_VCORE_TRACK_ON_CTRL, en);
	pmic_set_register_value(PMIC_RG_VSRAM_VCORE_TRACK_SLEEP_CTRL, en);

	if (pmic_get_register_value(PMIC_RG_VSRAM_VCORE_TRACK_VBUCK_ON_CTRL) == en &&
	    pmic_get_register_value(PMIC_RG_VSRAM_VCORE_TRACK_ON_CTRL) == en &&
	    pmic_get_register_value(PMIC_RG_VSRAM_VCORE_TRACK_SLEEP_CTRL) == en) {
		pr_err("[PMIC][%s] %s HW TRACKING success\n", __func__, (en == 1)?"enable":"disable");
		if (en == 0)
			buck_set_voltage(VSRAM_VCORE, 1000000);
		return 0;
	}
	pr_err("[PMIC][%s] %s HW TRACKING fail\n", __func__, (en == 1)?"enable":"disable");
	return 1;
}


/**********************************************************
  *
  *   [Internal Function]
  *
  *********************************************************/
/* dump exception reg in kernel and clean status */
int pmic_dump_exception_reg(void)
{
#if 0	/* TBD */
	int ret_val = 0;

	pr_err("[PMIC_dump_exception_reg][pmic_status]\n");
	kernel_dump_exception_reg();

	/* clear UVLO off */
	ret_val = pmic_config_interface(MT6351_TOP_RST_STATUS_CLR, 0xFFFF, 0xFFFF, 0);

	/* clear thermal shutdown 150 */
	ret_val = pmic_set_register_value(PMIC_RG_STRUP_THR_CLR, 0x1);
	udelay(200);
	ret_val = pmic_set_register_value(PMIC_RG_STRUP_THR_CLR, 0x0);

	/* clear power not good */
	ret_val = pmic_set_register_value(PMIC_STRUP_PG_STATUS_CLR, 0x1);
	udelay(200);
	ret_val = pmic_set_register_value(PMIC_STRUP_PG_STATUS_CLR, 0x0);

	/* clear Long press shutdown */
	ret_val = pmic_set_register_value(PMIC_CLR_JUST_RST, 0x1);
	udelay(200);
	ret_val = pmic_set_register_value(PMIC_CLR_JUST_RST, 0x0);
	udelay(200);
	kernel_output_reg(MT6351_STRUP_CON12);

	/* clear WDTRST */
	ret_val = pmic_config_interface(MT6351_TOP_RST_MISC_SET, 0x8, 0xFFFF, 0);
	udelay(100);
	ret_val = pmic_config_interface(MT6351_TOP_RST_MISC_CLR, 0x8, 0xFFFF, 0);

	/* clear BUCK OC */
	ret_val = pmic_config_interface(MT6351_BUCK_OC_CON0, 0xFFFF, 0xFFFF, 0);
	udelay(200);

	/* clear Additional(TBD) */

	/* add mdelay for output the log in buffer */
	mdelay(500);
	mdelay(500);
	mdelay(500);
	mdelay(500);
	mdelay(500);
#endif
	return 0;
}

void pmic_dump_register(struct seq_file *m)
{
	const PMU_FLAG_TABLE_ENTRY *pFlag = &pmu_flags_table[PMU_COMMAND_MAX - 1];
	unsigned int i = 0;

	PMICLOG("dump PMIC 6335 register\n");

	for (i = 0; i < (pFlag->offset - 10); i = i + 10) {
		PMICLOG("Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x\n",
			i, upmu_get_reg_value(i),
			i + 2, upmu_get_reg_value(i + 2),
			i + 4, upmu_get_reg_value(i + 4),
			i + 6, upmu_get_reg_value(i + 6),
			i + 8, upmu_get_reg_value(i + 8));
		if (m != NULL) {
			seq_printf(m,
				"Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x\n",
				i, upmu_get_reg_value(i),
				i + 2, upmu_get_reg_value(i + 2),
				i + 4, upmu_get_reg_value(i + 4),
				i + 6, upmu_get_reg_value(i + 6),
				i + 8, upmu_get_reg_value(i + 8));
		}
	}

}

#define DUMP_ALL_REG 0
int pmic_pre_wdt_reset(void)
{
	int ret = 0;

	preempt_disable();
	local_irq_disable();
#if 0	/* TBD */
	/* for olympus pre wdt reset */
	ret = pmic_set_register_value(PMIC_RG_VCORE_VSLEEP_SEL, 0x0);
	ret = pmic_set_register_value(PMIC_RG_VSRAM_MD_VSLEEP_SEL, 0x0);
	ret = pmic_set_register_value(PMIC_RG_VMODEM_VSLEEP_SEL, 0x0);
	ret = pmic_set_register_value(PMIC_RG_VMD1_VSLEEP_SEL, 0x0);

	ret = pmic_set_register_value(PMIC_BUCK_VCORE_VOSEL_SLEEP, 0x10);
	ret = pmic_set_register_value(PMIC_BUCK_VMD1_VOSEL_SLEEP, 0x10);
	ret = pmic_set_register_value(PMIC_BUCK_VMODEM_VOSEL_SLEEP, 0x10);
	ret = pmic_set_register_value(PMIC_BUCK_VSRAM_MD_VOSEL_SLEEP, 0x10);
#endif
	pmic_dump_exception_reg();
#if DUMP_ALL_REG
	pmic_dump_register();
#endif
	return ret;

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

/*****************************************************************************
 * mt-pmic dev_attr APIs
 ******************************************************************************/
unsigned int g_reg_value;
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

		pvalue = (char *)buf;
		if (size > 5) {
			addr = strsep(&pvalue, " ");
			ret = kstrtou32(addr, 16, (unsigned int *)&reg_address);
		} else
			ret = kstrtou32(pvalue, 16, (unsigned int *)&reg_address);

		if (size > 5) {
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
unsigned char g_reg_value_pmic;

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
unsigned char g_auxadc_pmic;

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
			for (j = 0; j < AUXADC_LIST_MAX; j++) {
				pr_err("[PMIC_AUXADC] [%s]=%d\n",
						pmic_get_auxadc_name(j),
						pmic_get_auxadc_value(j));
				mdelay(5);
			}
		}
	}
	return size;
}

static DEVICE_ATTR(pmic_auxadc_ut, 0664, show_pmic_auxadc, store_pmic_auxadc);

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
unsigned short is_battery_remove;
unsigned short is_wdt_reboot_pmic;
unsigned short is_wdt_reboot_pmic_chk;

unsigned short is_battery_remove_pmic(void)
{
	return is_battery_remove;
}

void PMIC_CUSTOM_SETTING_V1(void)
{
}

static int proc_dump_register_show(struct seq_file *m, void *v)
{
	seq_puts(m, "********** dump PMIC registers**********\n");
	pmic_dump_register(m);

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

struct dentry *mtk_pmic_dir;

static int pmic_debug_init(struct platform_device *dev)
{
	mtk_pmic_dir = debugfs_create_dir("mtk_pmic", NULL);
	if (!mtk_pmic_dir) {
		pr_err(PMICTAG "fail to mkdir /sys/kernel/debug/mtk_pmic\n");
		return -ENOMEM;
	}

	debugfs_create_file("dump_pmic_reg", S_IRUGO | S_IWUSR, mtk_pmic_dir,
		NULL, &pmic_dump_register_proc_fops);

	pmic_regulator_debug_init(dev, mtk_pmic_dir);
	pmic_throttling_dlpt_debug_init(dev, mtk_pmic_dir);
	PMICLOG("proc_create pmic_dump_register_proc_fops\n");

	return 0;
}

/*****************************************************************************
 * system function
 ******************************************************************************/
static int pmic_mt_probe(struct platform_device *dev)
{
	int ret_device_file = 0;

	PMICLOG("******** MT pmic driver probe!! ********\n");
	/*get PMIC CID */
	PMICLOG("PMIC CID = 0x%x\n", pmic_get_register_value(PMIC_SWCID));
	kernel_dump_exception_reg();
	/* upmu_set_reg_value(MT6335_TOP_RST_STATUS, 0xFF); TBD, written by Jeter*/

#if MT6799_BOOT_VOL
	/* For whitney only(set boot voltage in preloader) */
	if (pmic_get_register_value(PMIC_RG_BUCK_VMD1_VOSEL) == 0x48 ||
		pmic_get_register_value(PMIC_RG_BUCK_VMODEM_VOSEL) == 0x50 ||
		pmic_get_register_value(PMIC_RG_VSRAM_VMD_VOSEL) == 0x60) {
		vmd1_vosel = 0x48;	/* 0.85V */
		vmodem_vosel = 0x50;	/* 0.90V */
		vsram_vmd_vosel = 0x60;	/* 1.00V */
	}
#endif

	PMIC_INIT_SETTING_V1();
	PMICLOG("[PMIC_INIT_SETTING_V1] Done\n");

/*#if defined(CONFIG_FPGA_EARLY_PORTING)*/
#if 0
	PMICLOG("[PMIC_EINT_SETTING] disable when CONFIG_FPGA_EARLY_PORTING\n");
#else
	/*PMIC Interrupt Service*/
	PMIC_EINT_SETTING();
	PMICLOG("[PMIC_EINT_SETTING] Done\n");
#endif

	mtk_regulator_init(dev);

	pmic_throttling_dlpt_init();

	pmic_debug_init(dev);
	PMICLOG("[PMIC] pmic_debug_init : done.\n");

	pmic_ftm_init();

	ret_device_file = device_create_file(&(dev->dev), &dev_attr_pmic_access);
	ret_device_file = device_create_file(&(dev->dev), &dev_attr_pmic_dvt);
	ret_device_file = device_create_file(&(dev->dev), &dev_attr_pmic_auxadc_ut);
	PMICLOG("[PMIC] device_create_file for EM : done.\n");

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

	pmic_throttling_dlpt_suspend();
	return 0;
}

static int pmic_mt_resume(struct platform_device *dev)
{
	pmic_suspend_state = false;

	PMICLOG("******** MT pmic driver resume!! ********\n");

	pmic_throttling_dlpt_resume();
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
	/*#ifdef CONFIG_PM*/
	.suspend = pmic_mt_suspend,
	.resume = pmic_mt_resume,
	/*#endif*/
	.driver = {
		   .name = "mt-pmic",
	},
};

/*****************************************************************************
 * PMIC mudule init/exit
 ******************************************************************************/
static int __init pmic_mt_init(void)
{
	int ret;

	pmic_init_wake_lock(&pmicThread_lock, "pmicThread_lock wakelock");

	PMICLOG("pmic_regulator_init_OF\n");

	/* PMIC device driver register*/
	ret = platform_device_register(&pmic_mt_device);
	if (ret) {
		pr_err("****[pmic_mt_init] Unable to device register(%d)\n", ret);
		return ret;
	}
	ret = platform_driver_register(&pmic_mt_driver);
	if (ret) {
		pr_err("****[pmic_mt_init] Unable to register driver (%d)\n", ret);
		return ret;
	}

#ifdef CONFIG_MTK_AUXADC_INTF
	mtk_auxadc_init();
#else
	pmic_auxadc_init();
#endif /* CONFIG_MTK_AUXADC_INTF */
	pr_debug("****[pmic_mt_init] Initialization : DONE !!\n");

	return 0;
}

static void __exit pmic_mt_exit(void)
{
	platform_driver_unregister(&pmic_mt_driver);
}
fs_initcall(pmic_mt_init);
module_exit(pmic_mt_exit);

MODULE_AUTHOR("Jeter Chen");
MODULE_DESCRIPTION("MT PMIC Device Driver");
MODULE_LICENSE("GPL");

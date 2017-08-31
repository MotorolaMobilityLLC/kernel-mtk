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
#include "include/pmic_api_buck.h"
#include "include/pmic_bif.h"
#include "pwrap_hal.h"

#ifdef CONFIG_MTK_AUXADC_INTF
#include <mt-plat/mtk_auxadc_intf.h>
#endif /* CONFIG_MTK_AUXADC_INTF */

#if defined(CONFIG_MTK_SMART_BATTERY)
#include <mt-plat/battery_meter.h>
#include <mt-plat/battery_common.h>
#include <mach/mtk_battery_meter.h>
#endif

/*****************************************************************************
 * PMIC related define
 ******************************************************************************/
static DEFINE_MUTEX(pmic_lock_mutex);
#define PMIC_EINT_SERVICE

/*****************************************************************************
 * PMIC read/write APIs
 ******************************************************************************/
#ifdef CONFIG_FPGA_EARLY_PORTING /*defined(CONFIG_FPGA_EARLY_PORTING)*/
    /* no CONFIG_PMIC_HW_ACCESS_EN in FPGA */
#else
#ifdef CONFIG_MTK_PMIC_WRAP_HAL
#define CONFIG_PMIC_HW_ACCESS_EN
#else
    /* no CONFIG_PMIC_HW_ACCESS_EN in EVB/PHONE*/
#endif
#endif

/*---IPI Mailbox define---*/
/*#define IPIMB*/
#if defined(IPIMB)
#include <mach/mtk_pmic_ipi.h>
#endif
static DEFINE_MUTEX(pmic_access_mutex);
/*--- Global suspend state ---*/
static bool pmic_suspend_state;
#if defined(CONFIG_MACH_MT6759)
static int tdet_cnt;
#endif

/*
 * pmic intf
 */
unsigned int pmic_read_interface(unsigned int RegNum, unsigned int *val, unsigned int MASK, unsigned int SHIFT)
{
	unsigned int return_value = 0;

#if defined(CONFIG_PMIC_HW_ACCESS_EN)
	unsigned int pmic_reg = 0;
	unsigned int rdata;

	return_value = pwrap_read((RegNum), &rdata);
	pmic_reg = rdata;
	if (return_value != 0) {
		pr_err("[%s] err ret=%d, Reg[0x%x] pmic_wrap read data fail, MASK=0x%x, SHIFT=%d\n",
			 __func__, return_value, RegNum, MASK, SHIFT);
		return return_value;
	}

	pmic_reg &= (MASK << SHIFT);
	*val = (pmic_reg >> SHIFT);
	PMICLOG("[pmic_read_interface] (0x%x,0x%x,0x%x,0x%x)\n", RegNum, *val, MASK, SHIFT);
#else
	PMICLOG("[pmic_read_interface] Can not access HW PMIC(FPGA?/PWRAP?)\n");
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
		pr_err("[%s] err ret=%d, Reg[0x%x] pmic_wrap read data fail, val=0x%x, MASK=0x%x, SHIFT=%d\n",
			 __func__, return_value, RegNum, val, MASK, SHIFT);
		mutex_unlock(&pmic_access_mutex);
		return return_value;
	}

	pmic_reg &= ~(MASK << SHIFT);
	pmic_reg |= (val << SHIFT);

	PMICLOG("[pmic_config_interface] (0x%x,0x%x,0x%x,0x%x,0x%x)\n", RegNum, val, MASK, SHIFT, pmic_reg);

	return_value = pwrap_write((RegNum), pmic_reg);
	if (return_value != 0) {
		pr_err("[%s] err ret=%d, Reg[0x%x]=0x%x pmic_wrap write data fail, val=0x%x, MASK=0x%x, SHIFT=%d\n",
			 __func__, return_value, RegNum, pmic_reg, val, MASK, SHIFT);
		mutex_unlock(&pmic_access_mutex);
		return return_value;
	}

	mutex_unlock(&pmic_access_mutex);

#else /*---IPIMB---*/

#if defined(CONFIG_MACH_MT6759)
	if (RegNum == 0x2210 || RegNum == 0x220E)
		if (tdet_cnt++ > 0)
			WARN_ON(1);
#endif
	mutex_lock(&pmic_access_mutex);

	return_value = pmic_ipi_config_interface(RegNum, val, MASK, SHIFT, 1);

	if (return_value)
		pr_err("[%s]IPIMB write data fail with ret = %d, (0x%x,0x%x,0x%x,%d)\n", __func__,
			return_value, RegNum, val, MASK, SHIFT);
	else
		PMICLOG("[%s]IPIMB write data =(0x%x,0x%x,0x%x,%d)\n", __func__,
			RegNum, val, MASK, SHIFT);

	mutex_unlock(&pmic_access_mutex);
#endif /*---!IPIMB---*/

#else
	PMICLOG("[pmic_config_interface] Can not access HW PMIC(FPGA?/PWRAP?)\n");
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
		pr_err("[%s] err ret=%d, Reg[0x%x] pmic_wrap read data fail, val=0x%x, MASK=0x%x, SHIFT=%d\n",
			 __func__, return_value, RegNum, val, MASK, SHIFT);
		return return_value;
	}
	/*PMICLOG"[pmic_config_interface] Reg[%x]=0x%x\n", RegNum, pmic_reg); */

	pmic_reg &= ~(MASK << SHIFT);
	pmic_reg |= (val << SHIFT);
	PMICLOG("[pmic_config_interface_nolock] (0x%x,0x%x,0x%x,0x%x,0x%x)\n", RegNum, val, MASK, SHIFT, pmic_reg);

	return_value = pwrap_write((RegNum), pmic_reg);
	if (return_value != 0) {
		pr_err("[%s] err ret=%d, Reg[0x%x]=0x%x pmic_wrap write data fail, val=0x%x, MASK=0x%x, SHIFT=%d\n",
			 __func__, return_value, RegNum, pmic_reg, val, MASK, SHIFT);
		return return_value;
	}
	/*PMICLOG"[pmic_config_interface] write Reg[%x]=0x%x\n", RegNum, pmic_reg); */

#else /*---IPIMB---*/

#if defined(CONFIG_MACH_MT6759)
	if (RegNum == 0x2210 || RegNum == 0x220E)
		if (tdet_cnt++ > 0)
			WARN_ON(1);
#endif
	return_value = pmic_ipi_config_interface(RegNum, val, MASK, SHIFT, 0);

	if (return_value)
		pr_err("[%s]IPIMB write data fail with ret = %d, (0x%x,0x%x,0x%x,%d)\n", __func__,
			return_value, RegNum, val, MASK, SHIFT);
	else
		PMICLOG("[%s]IPIMB write data =(0x%x,0x%x,0x%x,%d)\n", __func__,
			RegNum, val, MASK, SHIFT);

#endif /*---IPIMB---*/


#else
	PMICLOG("[pmic_config_interface_nolock] Can not access HW PMIC(FPGA?/PWRAP?)\n");
#endif	/*defined(CONFIG_PMIC_HW_ACCESS_EN)*/

	return return_value;
}

unsigned short pmic_set_register_value(PMU_FLAGS_LIST_ENUM flagname, unsigned int val)
{
	const PMU_FLAG_TABLE_ENTRY *pFlag = &pmu_flags_table[flagname];
	unsigned int ret = 0;

	if (pFlag->flagname != flagname) {
		pr_err("[%s]pmic flag idx error\n", __func__);
		return 1;
	}

	ret = pmic_config_interface((pFlag->offset), (unsigned int)(val),
		(unsigned int)(pFlag->mask), (unsigned int)(pFlag->shift));
	if (ret != 0) {
		pr_err("[%s] error ret: %d when set Reg[0x%x]=0x%x\n", __func__,
			 ret, (pFlag->offset), val);
		return ret;
	}

	return 0;
}

unsigned short pmic_get_register_value(PMU_FLAGS_LIST_ENUM flagname)
{
	const PMU_FLAG_TABLE_ENTRY *pFlag = &pmu_flags_table[flagname];
	unsigned int val = 0;
	unsigned int ret = 0;

	ret = pmic_read_interface((unsigned int)pFlag->offset, &val,
		(unsigned int)(pFlag->mask), (unsigned int)(pFlag->shift));
	if (ret != 0) {
		pr_err("[%s] error ret: %d when get Reg[0x%x]\n", __func__,
			ret, (pFlag->offset));
		return ret;
	}

	return val;
}

unsigned short pmic_set_register_value_nolock(PMU_FLAGS_LIST_ENUM flagname, unsigned int val)
{
	const PMU_FLAG_TABLE_ENTRY *pFlag = &pmu_flags_table[flagname];
	unsigned int ret = 0;

	if (pFlag->flagname != flagname) {
		pr_err("[%s]pmic flag idx error\n", __func__);
		return 1;
	}

	ret = pmic_config_interface_nolock((pFlag->offset), (unsigned int)(val),
		(unsigned int)(pFlag->mask), (unsigned int)(pFlag->shift));
	if (ret != 0) {
		pr_err("[%s] error ret: %d when set Reg[0x%x]=0x%x\n", __func__,
			 ret, (pFlag->offset), val);
		return ret;
	}

	return 0;
}

unsigned short pmic_get_register_value_nolock(PMU_FLAGS_LIST_ENUM flagname)
{
	const PMU_FLAG_TABLE_ENTRY *pFlag = &pmu_flags_table[flagname];
	unsigned int val = 0;
	unsigned int ret = 0;

	ret = pmic_read_interface_nolock((unsigned int)pFlag->offset, &val,
		(unsigned int)(pFlag->mask), (unsigned int)(pFlag->shift));
	if (ret != 0) {
		pr_err("[%s] error ret: %d when get Reg[0x%x]\n", __func__,
			ret, (pFlag->offset));
		return ret;
	}

	return val;
}

unsigned short bc11_set_register_value(PMU_FLAGS_LIST_ENUM flagname, unsigned int val)
{
	return pmic_set_register_value(flagname, val);
}

unsigned short bc11_get_register_value(PMU_FLAGS_LIST_ENUM flagname)
{
	return pmic_get_register_value(flagname);
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

/*****************************************************************************
 * FTM
 ******************************************************************************/
#define PMIC_DEVNAME "pmic_ftm"
#define Get_IS_EXT_BUCK_EXIST _IOW('k', 20, int)
#define Get_IS_EXT_VBAT_BOOST_EXIST _IOW('k', 21, int)
#define Get_IS_EXT_SWCHR_EXIST _IOW('k', 22, int)
#define Get_IS_EXT_BUCK2_EXIST _IOW('k', 23, int)
#define Get_IS_EXT_BUCK3_EXIST _IOW('k', 24, int)


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
		/*adc_out_data[0] = is_ext_buck_gpio_exist();*/
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
	case Get_IS_EXT_BUCK3_EXIST:
		user_data_addr = (int *)arg;
		ret = copy_from_user(adc_in_data, user_data_addr, 8);
		adc_out_data[0] = 0;
		ret = copy_to_user(user_data_addr, adc_out_data, 8);
		PMICLOG("[pmic_ftm_ioctl] Get_IS_EXT_BUCK3_EXIST:%d\n", adc_out_data[0]);
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

/*****************************************************************************
 * system function
 ******************************************************************************/
static int pmic_mt_probe(struct platform_device *dev)
{
	PMICLOG("******** MT pmic driver probe!! ********\n");
	/*get PMIC CID */
	PMICLOG("PMIC CID = 0x%x\n", pmic_get_register_value(PMIC_SWCID));
	kernel_dump_exception_reg();

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

#ifdef CONFIG_MTK_AUXADC_INTF
	mtk_auxadc_init();
#else
	pmic_auxadc_init();
#endif /* CONFIG_MTK_AUXADC_INTF */
	pr_debug("****[pmic_mt_init] Initialization : DONE !!\n");

	pmic_throttling_dlpt_init();

	pmic_debug_init(dev);
	PMICLOG("[PMIC] pmic_debug_init : done.\n");

	pmic_ftm_init();

	if (IS_ENABLED(CONFIG_MTK_BIF_SUPPORT))
		pmic_bif_init();

	pmic_tracking_init();

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

	pr_debug("****[pmic_mt_init] Initialization : DONE !!\n");

	return 0;
}

static void __exit pmic_mt_exit(void)
{
	platform_driver_unregister(&pmic_mt_driver);
}
fs_initcall(pmic_mt_init);
module_exit(pmic_mt_exit);

MODULE_AUTHOR("Jimmy-YJ Huang");
MODULE_DESCRIPTION("MTK PMIC COMMON Interface Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0_M");

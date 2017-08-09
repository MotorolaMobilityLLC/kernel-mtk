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
#include <mt-plat/aee.h>
#include <linux/proc_fs.h>
#include <linux/debugfs.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/writeback.h>
#include <linux/seq_file.h>
#include <linux/time.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/regulator/of_regulator.h>
#include <linux/of_device.h>
#include <linux/of_fdt.h>
#endif
#include <linux/uaccess.h>

#if !defined CONFIG_MTK_LEGACY
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#endif

#include "include/sub_upmu_hw.h"
#include "include/sub_pmic.h"
#include "include/sub_pmic_irq.h"
/*TBD*/
/*#include <mach/mt_pmic_wrap.h>*/
#include "pwrap_hal.h"

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

#define pwrap_wacs2_read(a, b) (0)
#define pwrap_wacs2_write(a, b) (0)

/*---IPI Mailbox define---*/
/*
#define IPIMB
*/
static DEFINE_MUTEX(mt6337_access_mutex);

/*--- Global suspend state ---*/
static bool pmic_suspend_state_mt6337;

unsigned int mt6337_read_interface(unsigned int RegNum, unsigned int *val, unsigned int MASK, unsigned int SHIFT)
{
	unsigned int return_value = 0;

#if defined(CONFIG_PMIC_HW_ACCESS_EN)
#ifdef IPIMB
	*val = 0;
	PMICLOG("[mt6337_read_interface] IPIMB\n");
	/*PMICLOG"[mt6337_read_interface] val=0x%x\n", *val);*/
#else
	unsigned int pmic_reg = 0;
	unsigned int rdata = 0;

	if ((pmic_suspend_state_mt6337 == true) && irqs_disabled())
		return mt6337_read_interface_nolock(RegNum, val, MASK, SHIFT);

	mutex_lock(&mt6337_access_mutex);

	/*mt_read_byte(RegNum, &pmic_reg);*/
	/*return_value = pwrap_wacs2(0, (RegNum), 0, &rdata);*/
	return_value = pwrap_wacs2_read((RegNum), &rdata);
	pmic_reg = rdata;
	if (return_value != 0) {
		PMICLOG("[mt6337_read_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
		mutex_unlock(&mt6337_access_mutex);
		return return_value;
	}
	/*PMICLOG"[mt6337_read_interface] Reg[%x]=0x%x\n", RegNum, pmic_reg);*/

	pmic_reg &= (MASK << SHIFT);
	*val = (pmic_reg >> SHIFT);
	/*PMICLOG"[mt6337_read_interface] val=0x%x\n", *val);*/

#endif /*---IPIMB---*/
#else
	/*PMICLOG("[mt6337_read_interface] Can not access HW PMIC\n");*/
#endif	/*defined(CONFIG_PMIC_HW_ACCESS_EN)*/

	return return_value;
}

unsigned int mt6337_config_interface(unsigned int RegNum, unsigned int val, unsigned int MASK, unsigned int SHIFT)
{
	unsigned int return_value = 0;

#if defined(CONFIG_PMIC_HW_ACCESS_EN)
#ifdef IPIMB
#else
	unsigned int pmic_reg = 0;
	unsigned int rdata = 0;

	if ((pmic_suspend_state_mt6337 == true) && irqs_disabled())
		return mt6337_config_interface_nolock(RegNum, val, MASK, SHIFT);

	mutex_lock(&mt6337_access_mutex);

	/*1. mt_read_byte(RegNum, &pmic_reg);*/
	/*return_value = pwrap_wacs2(0, (RegNum), 0, &rdata);*/
	return_value = pwrap_wacs2_read((RegNum), &rdata);
	pmic_reg = rdata;
	if (return_value != 0) {
		PMICLOG("[mt6337_config_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
		mutex_unlock(&mt6337_access_mutex);
		return return_value;
	}
	/*PMICLOG"[mt6337_config_interface] Reg[%x]=0x%x\n", RegNum, pmic_reg);*/

	pmic_reg &= ~(MASK << SHIFT);
	pmic_reg |= (val << SHIFT);

	/*2. mt_write_byte(RegNum, pmic_reg);*/
	/*return_value = pwrap_wacs2(1, (RegNum), pmic_reg, &rdata);*/
	return_value = pwrap_wacs2_write((RegNum), pmic_reg);
	if (return_value != 0) {
		PMICLOG("[mt6337_config_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
		mutex_unlock(&mt6337_access_mutex);
		return return_value;
	}
	/*PMICLOG"[mt6337_config_interface] write Reg[%x]=0x%x\n", RegNum, pmic_reg);*/
#if 0
	/*3. Double Check*/
	/*mt_read_byte(RegNum, &pmic_reg);*/
	return_value = pwrap_wacs2(0, (RegNum), 0, &rdata);
	pmic_reg = rdata;
	if (return_value != 0) {
		PMICLOG("[mt6337_config_interface] Reg[%x]= pmic_wrap write data fail\n", RegNum);
		mutex_unlock(&pmic_access_mutex);
		return return_value;
	}
	PMICLOG("[mt6337_config_interface] Reg[%x]=0x%x\n", RegNum, pmic_reg);
#endif

	mutex_unlock(&mt6337_access_mutex);
#endif /*---IPIMB---*/
#else
	/*PMICLOG("[mt6337_config_interface] Can not access HW PMIC\n");*/
#endif	/*defined(CONFIG_PMIC_HW_ACCESS_EN)*/

	return return_value;
}

unsigned int mt6337_read_interface_nolock(unsigned int RegNum, unsigned int *val,
	unsigned int MASK, unsigned int SHIFT)
{
	unsigned int return_value = 0;

#if defined(CONFIG_PMIC_HW_ACCESS_EN)
#ifdef IPIMB
	*val = 0;
	PMICLOG("[mt6337_read_interface_nolock] IPIMB\n");
#else
	unsigned int pmic_reg = 0;
	unsigned int rdata = 0;

	/*mt_read_byte(RegNum, &pmic_reg); */
	/*return_value = pwrap_wacs2(0, (RegNum), 0, &rdata);*/
	return_value = pwrap_wacs2_read((RegNum), &rdata);
	pmic_reg = rdata;
	if (return_value != 0) {
		PMICLOG("[mt6337_read_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
		return return_value;
	}
	/*PMICLOG"[mt6337_read_interface] Reg[%x]=0x%x\n", RegNum, pmic_reg); */

	pmic_reg &= (MASK << SHIFT);
	*val = (pmic_reg >> SHIFT);
	/*PMICLOG"[mt6337_read_interface] val=0x%x\n", *val); */
#endif /*---IPIMB---*/
#else
	/*PMICLOG("[mt6337_read_interface] Can not access HW PMIC\n"); */
#endif	/*defined(CONFIG_PMIC_HW_ACCESS_EN)*/

	return return_value;
}

unsigned int mt6337_config_interface_nolock(unsigned int RegNum, unsigned int val,
	unsigned int MASK, unsigned int SHIFT)
{
	unsigned int return_value = 0;

#if defined(CONFIG_PMIC_HW_ACCESS_EN)
#ifdef IPIMB
#else
	unsigned int pmic_reg = 0;
	unsigned int rdata = 0;

    /* pmic wrapper has spinlock protection. pmic do not to do it again */

	/*1. mt_read_byte(RegNum, &pmic_reg); */
	/*return_value = pwrap_wacs2(0, (RegNum), 0, &rdata);*/
	return_value = pwrap_wacs2_read((RegNum), &rdata);
	pmic_reg = rdata;
	if (return_value != 0) {
		PMICLOG("[mt6337_config_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
		return return_value;
	}
	/*PMICLOG"[mt6337_config_interface] Reg[%x]=0x%x\n", RegNum, pmic_reg); */

	pmic_reg &= ~(MASK << SHIFT);
	pmic_reg |= (val << SHIFT);

	/*2. mt_write_byte(RegNum, pmic_reg); */
	/*return_value = pwrap_wacs2(1, (RegNum), pmic_reg, &rdata);*/
	return_value = pwrap_wacs2_write((RegNum), pmic_reg);
	if (return_value != 0) {
		PMICLOG("[mt6337_config_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
		return return_value;
	}
	/*PMICLOG"[mt6337_config_interface] write Reg[%x]=0x%x\n", RegNum, pmic_reg); */

#if 0
	/*3. Double Check */
	/*mt_read_byte(RegNum, &pmic_reg); */
	return_value = pwrap_wacs2(0, (RegNum), 0, &rdata);
	pmic_reg = rdata;
	if (return_value != 0) {
		PMICLOG("[mt6337_config_interface] Reg[%x]= pmic_wrap write data fail\n", RegNum);
		mutex_unlock(&pmic_access_mutex);
		return return_value;
	}
	PMICLOG("[mt6337_config_interface] Reg[%x]=0x%x\n", RegNum, pmic_reg);
#endif
#endif /*---IPIMB---*/

#else
	/*PMICLOG("[mt6337_config_interface] Can not access HW PMIC\n"); */
#endif	/*defined(CONFIG_PMIC_HW_ACCESS_EN)*/

	return return_value;
}

/*****************************************************************************
 * PMIC lock/unlock APIs
 ******************************************************************************/
unsigned int mt6337_upmu_get_reg_value(unsigned int reg)
{
	unsigned int reg_val = 0;

#ifdef IPIMB
	reg_val = 0;
	PMICLOG("[mt6337_read_interface] IPIMB\n");
#else
	unsigned int ret = 0;

	ret = mt6337_read_interface(reg, &reg_val, 0xFFFF, 0x0);
#endif
	return reg_val;
}
EXPORT_SYMBOL(mt6337_upmu_get_reg_value);

void mt6337_upmu_set_reg_value(unsigned int reg, unsigned int reg_val)
{
	unsigned int ret = 0;

	ret = mt6337_config_interface(reg, reg_val, 0xFFFF, 0x0);
}

/**********************************************************
  *
  *   [Internal Function]
  *
  *********************************************************/
void mt6337_dump_register(void)
{
	unsigned int i = 0;

	PMICLOG("dump PMIC 6337 register\n");

	for (i = 0x8000; i <= 0x9e00; i = i + 10) {
		pr_debug
		    ("Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x\n",
		     i, mt6337_upmu_get_reg_value(i), i + 1, mt6337_upmu_get_reg_value(i + 1), i + 2,
		     mt6337_upmu_get_reg_value(i + 2), i + 3, mt6337_upmu_get_reg_value(i + 3), i + 4,
		     mt6337_upmu_get_reg_value(i + 4));

		pr_debug
		    ("Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x\n",
		     i + 5, mt6337_upmu_get_reg_value(i + 5), i + 6, mt6337_upmu_get_reg_value(i + 6), i + 7,
		     mt6337_upmu_get_reg_value(i + 7), i + 8, mt6337_upmu_get_reg_value(i + 8), i + 9,
		     mt6337_upmu_get_reg_value(i + 9));
	}

}

/*
 * pmic_access attr
 */
unsigned int g_reg_value_6337 = 0;
static ssize_t show_mt6337_access(struct device *dev, struct device_attribute *attr, char *buf)
{
	pr_err("[show_mt6337_access] 0x%x\n", g_reg_value_6337);
	return sprintf(buf, "%u\n", g_reg_value_6337);
}

static ssize_t store_mt6337_access(struct device *dev, struct device_attribute *attr, const char *buf,
				 size_t size)
{
	int ret = 0;
	char *pvalue = NULL, *addr, *val;
	unsigned int reg_value = 0;
	unsigned int reg_address = 0;

	pr_err("[store_mt6337_access]\n");
	if (buf != NULL && size != 0) {
		pr_err("[store_mt6337_access] buf is %s\n", buf);
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

			pr_err("[store_mt6337_access] write PMU reg 0x%x with value 0x%x !\n",
				reg_address, reg_value);
			ret = mt6337_config_interface(reg_address, reg_value, 0xFFFF, 0x0);
		} else {
			ret = mt6337_read_interface(reg_address, &g_reg_value_6337, 0xFFFF, 0x0);
			pr_err("[store_mt6337_access] read PMU reg 0x%x with value 0x%x !\n",
				reg_address, g_reg_value_6337);
			pr_err("[store_mt6337_access] use \"cat mt6337_access\" to get value(decimal)\r\n");
		}
	}
	return size;
}

static DEVICE_ATTR(mt6337_access, 0664, show_mt6337_access, store_mt6337_access);	/*664*/

/*
 * auxadc attr
 */
unsigned char g_auxadc_mt6337 = 0;

static ssize_t show_mt6337_auxadc(struct device *dev, struct device_attribute *attr, char *buf)
{
	pr_err("[show_mt6337_auxadc] 0x%x\n", g_auxadc_mt6337);
	return sprintf(buf, "%u\n", g_auxadc_mt6337);
}

static ssize_t store_mt6337_auxadc(struct device *dev, struct device_attribute *attr, const char *buf,
			      size_t size)
{
	int ret = 0, i;
#if 0   /*--TBD--*/
	int j;
#endif
	char *pvalue = NULL;
	unsigned int val = 0;

	pr_err("[store_mt6337_auxadc]\n");

	if (buf != NULL && size != 0) {
		pvalue = (char *)buf;
		ret = kstrtou32(pvalue, 16, (unsigned int *)&val);
		for (i = 0; i < val; i++) {
#if 0   /*--TBD--*/
			for (j = 0; j < 16; j++) {
				pr_err("[MT6337_AUXADC] [%d]=%d\n", j, PMIC_IMM_GetOneChannelValue(j, 0, 0));
				mdelay(5);
			}
			pr_err("[MT6337_AUXADC] [%d]=%d\n", j, PMIC_IMM_GetOneChannelValue(PMIC_AUX_CH4_DCXO, 0, 0));
#endif
			pr_err("[MT6337_AUXADC] need to be impletement\n");
		}
	}
	return size;
}

static DEVICE_ATTR(mt6337_auxadc_ut, 0664, show_mt6337_auxadc, store_mt6337_auxadc);


/*****************************************************************************
 * HW Setting
 ******************************************************************************/
static int proc_dump_register_show(struct seq_file *m, void *v)
{
	int i;

	seq_puts(m, "********** dump MT6337 registers**********\n");

	for (i = 0x8000; i <= 0x9e00; i = i + 10) {
		seq_printf(m, "Reg[%x]=0x%x Reg[%x]=0x%x Reg[%x]=0x%x Reg[%x]=0x%x Reg[%x]=0x%x\n",
			i, mt6337_upmu_get_reg_value(i), i + 1, mt6337_upmu_get_reg_value(i + 1), i + 2,
			mt6337_upmu_get_reg_value(i + 2), i + 3, mt6337_upmu_get_reg_value(i + 3), i + 4,
			mt6337_upmu_get_reg_value(i + 4));

		seq_printf(m, "Reg[%x]=0x%x Reg[%x]=0x%x Reg[%x]=0x%x Reg[%x]=0x%x Reg[%x]=0x%x\n",
			i + 5, mt6337_upmu_get_reg_value(i + 5), i + 6, mt6337_upmu_get_reg_value(i + 6),
			i + 7, mt6337_upmu_get_reg_value(i + 7), i + 8, mt6337_upmu_get_reg_value(i + 8),
			i + 9, mt6337_upmu_get_reg_value(i + 9));
	}

	return 0;
}

static int proc_dump_mt6337_register_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_dump_register_show, NULL);
}

static const struct file_operations mt6337_dump_register_proc_fops = {
	.open = proc_dump_mt6337_register_open,
	.read = seq_read,
};

void mt6337_debug_init(struct platform_device *dev)
{
	struct dentry *mt6337_dir;

	mt6337_dir = debugfs_create_dir("mt6337", NULL);
	if (!mt6337_dir) {
		PMICLOG("fail to mkdir /sys/kernel/debug/mt6337\n");
		return;
	}

	debugfs_create_file("dump_mt6337_reg", S_IRUGO | S_IWUSR, mt6337_dir, NULL, &mt6337_dump_register_proc_fops);

	PMICLOG("proc_create mt6337_dump_register_proc_fops\n");
}

/*****************************************************************************
 * system function
 ******************************************************************************/
static int mt6337_probe(struct platform_device *dev)
{
	int ret_device_file = 0;

	PMICLOG("******** MT6337 driver probe!! ********\n");
	/*get PMIC CID */
	pr_debug
	    ("PMIC CID=0x%x PowerGoodStatus = 0x%x OCStatus = 0x%x ThermalStatus = 0x%x rsvStatus = 0x%x\n",
	     mt6337_get_register_value(MT6337_PMIC_SWCID), mt6337_upmu_get_reg_value(0x21c),
	     mt6337_upmu_get_reg_value(0x214), mt6337_upmu_get_reg_value(0x21e), mt6337_upmu_get_reg_value(0x2a6));

	/* mt6337_upmu_set_reg_value(0x2a6, 0xff); */ /* TBD */

	/*pmic initial setting */
#if defined(CONFIG_MTK_PMIC_CHIP_MT6337)
	PMICLOG("[MT6337_INIT_SETTING_V1] Done\n");
#else
	PMICLOG("[MT6337_INIT_SETTING_V1] delay to MT6311 init\n");
#endif


#if 0
	PMICLOG("[PMIC_EINT_SETTING] disable when CONFIG_MTK_FPGA\n");
#else
	/*PMIC Interrupt Service*/
	mt6337_thread_handle = kthread_create(mt6337_thread_kthread, (void *)NULL, "mt6337_thread");
	if (IS_ERR(mt6337_thread_handle)) {
		mt6337_thread_handle = NULL;
		PMICLOG("[mt6337_thread_kthread_mt6325] creation fails\n");
	} else {
		wake_up_process(mt6337_thread_handle);
		PMICLOG("[mt6337_thread_kthread_mt6325] kthread_create Done\n");
	}

	MT6337_EINT_SETTING();
	PMICLOG("[MT6337_EINT_SETTING] Done\n");
#endif

	mt6337_debug_init(dev);
	PMICLOG("[MT6337] mt6337_debug_init : done.\n");

	ret_device_file = device_create_file(&(dev->dev), &dev_attr_mt6337_access);
	ret_device_file = device_create_file(&(dev->dev), &dev_attr_mt6337_auxadc_ut);

	PMICLOG("[PMIC] device_create_file for EM : done.\n");

	/*pwrkey_sw_workaround_init(); */
	return 0;
}

static int mt6337_remove(struct platform_device *dev)
{
	PMICLOG("******** MT6337 pmic driver remove!! ********\n");

	return 0;
}

static void mt6337_shutdown(struct platform_device *dev)
{
	PMICLOG("******** MT6337 pmic driver shutdown!! ********\n");
}

static int mt6337_suspend(struct platform_device *dev, pm_message_t state)
{
	pmic_suspend_state_mt6337 = true;

	PMICLOG("******** MT6337 pmic driver suspend!! ********\n");

	return 0;
}

static int mt6337_resume(struct platform_device *dev)
{
	pmic_suspend_state_mt6337 = false;

	PMICLOG("******** MT6337 driver resume!! ********\n");

	return 0;
}

struct platform_device mt6337_device = {
	.name = "mt6337-pmic",
	.id = -1,
};

static struct platform_driver mt6337_driver = {
	.probe = mt6337_probe,
	.remove = mt6337_remove,
	.shutdown = mt6337_shutdown,
	/*#ifdef CONFIG_PM*/
	.suspend = mt6337_suspend,
	.resume = mt6337_resume,
	/*#endif*/
	.driver = {
		   .name = "mt6337-pmic",
		   },
};

/*****************************************************************************
 * PMIC mudule init/exit
 ******************************************************************************/
static int __init mt6337_init(void)
{
	int ret;

#if !defined CONFIG_HAS_WAKELOCKS
	wakeup_source_init(&pmicThread_lock_mt6337, "pmicThread_lock_mt6337 wakelock");
#else
	wake_lock_init(&pmicThread_lock_mt6337, WAKE_LOCK_SUSPEND, "pmicThread_lock_mt6337 wakelock");
#endif
	/* PMIC device driver register*/
	ret = platform_device_register(&mt6337_device);
	if (ret) {
		PMICLOG("****[mt6337_init] Unable to device register(%d)\n", ret);
		return ret;
	}
	ret = platform_driver_register(&mt6337_driver);
	if (ret) {
		PMICLOG("****[mt6337_init] Unable to register driver (%d)\n", ret);
		return ret;
	}

	pr_debug("****[mt6337_init] Initialization : DONE !!\n");

	return 0;
}

static void __exit mt6337_exit(void)
{
#if !defined CONFIG_MTK_LEGACY
#ifdef CONFIG_OF
	/*platform_driver_unregister(&mt_pmic_driver);*/
	platform_driver_unregister(&mt6337_driver);
#endif
#endif				/* End of #if !defined CONFIG_MTK_LEGACY */
}
/*fs_initcall(mt6337_init);*/

module_init(mt6337_init);
module_exit(mt6337_exit);

MODULE_AUTHOR("Jimmy-YJ Huang");
MODULE_DESCRIPTION("MT6337 PMIC Device Driver");
MODULE_LICENSE("GPL");

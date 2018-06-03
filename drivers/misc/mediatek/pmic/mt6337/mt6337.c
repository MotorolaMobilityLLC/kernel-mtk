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
#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/preempt.h>

#include "mt6337.h"
#include "pwrap_hal.h"

/*****************************************************************************
 * PMIC read/write APIs
 ******************************************************************************/
#if 0	/*defined(CONFIG_FPGA_EARLY_PORTING)*/
    /* no CONFIG_PMIC_HW_ACCESS_EN */
#else
#define CONFIG_PMIC_HW_ACCESS_EN
#endif

/*---IPI Mailbox define---*/
/*#define IPIMB*/
static DEFINE_MUTEX(mt6337_access_mutex);

/*--- Global suspend state ---*/
static bool pmic_suspend_state_mt6337;

unsigned int mt6337_read_interface(unsigned int RegNum, unsigned int *val, unsigned int MASK, unsigned int SHIFT)
{
	int return_value = 0;

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
	/*MT6337LOG"[mt6337_read_interface] Reg[%x]=0x%x\n", RegNum, pmic_reg);*/

	pmic_reg &= (MASK << SHIFT);
	*val = (pmic_reg >> SHIFT);
	/*MT6337LOG"[mt6337_read_interface] val=0x%x\n", *val);*/
#else
	/*MT6337LOG("[mt6337_read_interface] Can not access HW PMIC\n");*/
#endif	/*defined(CONFIG_PMIC_HW_ACCESS_EN)*/

	return return_value;
}

unsigned int mt6337_config_interface(unsigned int RegNum, unsigned int val, unsigned int MASK, unsigned int SHIFT)
{
	unsigned int return_value = 0;

#if defined(CONFIG_PMIC_HW_ACCESS_EN)
#ifndef IPIMB
	unsigned int pmic_reg = 0;
	unsigned int rdata;
#endif
	if (preempt_count() > 0 || irqs_disabled() || system_state != SYSTEM_RUNNING || oops_in_progress)
		return mt6337_config_interface_nolock(RegNum, val, MASK, SHIFT);
#ifndef IPIMB
	mutex_lock(&mt6337_access_mutex);
	return_value = pwrap_read((RegNum), &rdata);
	pmic_reg = rdata;
	if (return_value != 0) {
		pr_err("[%s] Reg[0x%x] val=0x%x MASK=0x%x SHIFT=%d pmic_wrap read data fail\n",
			__func__, RegNum, val, MASK, SHIFT);
		pr_err("[%s] PWRAP Error return value=%d\n", __func__, return_value);
		mutex_unlock(&mt6337_access_mutex);
		return return_value;
	}
	/*MT6337LOG"[mt6337_config_interface] Reg[%x]=0x%x\n", RegNum, pmic_reg);*/

	pmic_reg &= ~(MASK << SHIFT);
	pmic_reg |= (val << SHIFT);

	return_value = pwrap_write((RegNum), pmic_reg);
	if (return_value != 0) {
		pr_err("[%s] Reg[0x%x] val=0x%x MASK=0x%x SHIFT=%d pmic_wrap write data fail\n",
			__func__, RegNum, val, MASK, SHIFT);
		pr_err("[%s] PWRAP Error return value=%d\n", __func__, return_value);
		mutex_unlock(&mt6337_access_mutex);
		return return_value;
	}
	/*MT6337LOG"[mt6337_config_interface] write Reg[%x]=0x%x\n", RegNum, pmic_reg);*/

	mutex_unlock(&mt6337_access_mutex);
#else	/*---IPIMB---*/

#endif	/*---IPIMB---*/

#else
	/*MT6337LOG("[mt6337_config_interface] Can not access HW PMIC\n");*/
#endif	/*defined(CONFIG_PMIC_HW_ACCESS_EN)*/

	return return_value;
}

unsigned int mt6337_config_interface_nolock(unsigned int RegNum, unsigned int val,
	unsigned int MASK, unsigned int SHIFT)
{
	unsigned int return_value = 0;

#if defined(CONFIG_PMIC_HW_ACCESS_EN)
#ifndef IPIMB
	unsigned int pmic_reg = 0;
	unsigned int rdata;

	return_value = pwrap_read((RegNum), &rdata);
	pmic_reg = rdata;
	if (return_value != 0) {
		pr_err("[%s] Reg[0x%x] val=0x%x MASK=0x%x SHIFT=%d pmic_wrap read data fail\n",
			__func__, RegNum, val, MASK, SHIFT);
		pr_err("[%s] PWRAP Error return value=%d\n", __func__, return_value);
		return return_value;
	}
	/*MT6337LOG"[mt6337_config_interface] Reg[%x]=0x%x\n", RegNum, pmic_reg); */

	pmic_reg &= ~(MASK << SHIFT);
	pmic_reg |= (val << SHIFT);

	return_value = pwrap_write((RegNum), pmic_reg);
	if (return_value != 0) {
		pr_err("[%s] Reg[0x%x] val=0x%x MASK=0x%x SHIFT=%d pmic_wrap write data fail\n",
			__func__, RegNum, val, MASK, SHIFT);
		pr_err("[%s] PWRAP Error return value=%d\n", __func__, return_value);
		return return_value;
	}
	/*MT6337LOG"[mt6337_config_interface] write Reg[%x]=0x%x\n", RegNum, pmic_reg); */
#else /*---IPIMB---*/

#endif /*---IPIMB---*/

#else
	/*MT6337LOG("[mt6337_config_interface] Can not access HW PMIC\n"); */
#endif	/*defined(CONFIG_PMIC_HW_ACCESS_EN)*/

	return return_value;
}

unsigned int mt6337_upmu_get_reg_value(unsigned int reg)
{
	unsigned int reg_val = 0;
	unsigned int ret = 0;

	ret = mt6337_read_interface(reg, &reg_val, 0xFFFF, 0x0);
	return reg_val;
}
EXPORT_SYMBOL(mt6337_upmu_get_reg_value);

unsigned int mt6337_upmu_set_reg_value(unsigned int reg, unsigned int reg_val)
{
	unsigned int ret = 0;

	ret = mt6337_config_interface(reg, reg_val, 0xFFFF, 0x0);
	return ret;
}

/**********************************************************
  *
  *   [Internal Function]
  *
  *********************************************************/
void mt6337_dump_register(struct seq_file *m)
{
	const MT6337_PMU_FLAG_TABLE_ENTRY *pFlag_S = &mt6337_pmu_flags_table[0];
	const MT6337_PMU_FLAG_TABLE_ENTRY *pFlag_E = &mt6337_pmu_flags_table[MT6337_PMU_COMMAND_MAX - 1];
	unsigned int i = 0;

	MT6337LOG("dump PMIC 6337 register\n");

	for (i = (pFlag_S->offset); i <= (pFlag_E->offset - 10); i = i + 10) {
		MT6337LOG("Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x\n",
			i, mt6337_upmu_get_reg_value(i),
			i + 2, mt6337_upmu_get_reg_value(i + 2),
			i + 4, mt6337_upmu_get_reg_value(i + 4),
			i + 6, mt6337_upmu_get_reg_value(i + 6),
			i + 8, mt6337_upmu_get_reg_value(i + 8));
		if (m != NULL) {
			seq_printf(m,
				"Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x Reg[0x%x]=0x%x\n",
				i, mt6337_upmu_get_reg_value(i),
				i + 2, mt6337_upmu_get_reg_value(i + 2),
				i + 4, mt6337_upmu_get_reg_value(i + 4),
				i + 6, mt6337_upmu_get_reg_value(i + 6),
				i + 8, mt6337_upmu_get_reg_value(i + 8));
		}
	}
}

/*
 * pmic_access attr
 */
unsigned int g_reg_value_6337;
static ssize_t show_mt6337_access(struct device *dev, struct device_attribute *attr, char *buf)
{
	pr_err("[show_mt6337_access] 0x%x\n", g_reg_value_6337);
	return sprintf(buf, "0x%x\n", g_reg_value_6337);
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
		pr_err("[store_mt6337_access] size is %d, buf is %s\n", (int)size, buf);

		pvalue = (char *)buf;
		addr = strsep(&pvalue, " ");
		val = strsep(&pvalue, " ");
		if (addr)
			ret = kstrtou32(addr, 16, (unsigned int *)&reg_address);
		if (val) {
			ret = kstrtou32(val, 16, (unsigned int *)&reg_value);
			pr_err("[store_mt6337_access] write PMU reg 0x%x with value 0x%x !\n",
				reg_address, reg_value);
			ret = mt6337_config_interface(reg_address, reg_value, 0xFFFF, 0x0);
		} else {
			ret = mt6337_read_interface(reg_address, &g_reg_value_6337, 0xFFFF, 0x0);
			pr_err("[store_mt6337_access] read PMU reg 0x%x with value 0x%x !\n",
				reg_address, g_reg_value_6337);
			pr_err("[store_mt6337_access] use \"cat mt6337_access\" to get value\n");
		}
	}
	return size;
}

static DEVICE_ATTR(mt6337_access, 0664, show_mt6337_access, store_mt6337_access);	/*664*/

/*
 * auxadc attr
 */
unsigned char g_auxadc_mt6337;

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
	seq_puts(m, "********** dump MT6337 registers**********\n");
	mt6337_dump_register(m);

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
		pr_err("fail to mkdir /sys/kernel/debug/mt6337\n");
		return;
	}

	debugfs_create_file("dump_mt6337_reg", S_IRUGO | S_IWUSR, mt6337_dir,
		NULL, &mt6337_dump_register_proc_fops);

	MT6337LOG("proc_create mt6337_dump_register_proc_fops\n");
}

/*****************************************************************************
 * system function
 ******************************************************************************/
void MT6337_INIT_SETTING(void)
{
	unsigned int ret = 0;

	/* [4:4]: RG_SRCLKEN_IN0_HW_MODE; Joseph/ OSC CLK wi SRCLKEN */
	ret = mt6337_config_interface(0x8006, 0x1, 0x1, 4);
	/* [5:5]: RG_VOWEN_HW_MODE; Joseph/ OSC CLK wi SRCLKEN */
	ret = mt6337_config_interface(0x8006, 0x1, 0x1, 5);
	/* [6:6]: RG_OSC_SEL_HW_MODE; Joseph/ OSC CLK wi SRCLKEN */
	ret = mt6337_config_interface(0x8006, 0x1, 0x1, 6);
	/* [4:4]: RG_TRIM_75K_CK_PDN; Joseph/power down (FT trim use) */
	ret = mt6337_config_interface(0x8200, 0x1, 0x1, 4);
	/* [8:8]: RG_AUXADC_1M_CK_PDN; Joseph/power down */
	ret = mt6337_config_interface(0x8200, 0x1, 0x1, 8);
	/* [11:11]: RG_AUDIF_CK_PDN; Joseph/power down */
	ret = mt6337_config_interface(0x8200, 0x1, 0x1, 11);
	/* [3:3]: RG_INTRP_PRE_OC_CK_PDN; Joseph/power down (37 no use) */
	ret = mt6337_config_interface(0x8206, 0x1, 0x1, 3);
	/* [8:8]: RG_LDO_CALI_75K_CK_PDN; Joseph/power down (37 no use) */
	ret = mt6337_config_interface(0x8206, 0x1, 0x1, 8);
	/* [11:11]: RG_ACCDET_CK_PDN; Joseph/power on (ACCDET use) */
	ret = mt6337_config_interface(0x8206, 0x0, 0x1, 11);
	/* [7:7]: RG_REG_CK_PDN_HWEN; Joseph/RG CLK HW */
	ret = mt6337_config_interface(0x8218, 0x1, 0x1, 7);
	/* [8:8]: RG_OSC_SEL_BUCK_LDO_EN; Joseph/OSC CLK  mode don't care this signal */
	ret = mt6337_config_interface(0x8232, 0x0, 0x1, 8);
	/* [1:1]: RG_VA18_HW0_OP_EN; Joseph/LDO LP wi SRCLKEN */
	ret = mt6337_config_interface(0x9008, 0x1, 0x1, 1);
	/* [1:1]: RG_VA18_HW0_OP_CFG; Joseph/LDO LP wi SRCLKEN */
	ret = mt6337_config_interface(0x900E, 0x1, 0x1, 1);
	/* [15:15]: AUXADC_CK_AON; Juinn-Ting. Move from golden settings to initial settings as MT6335. */
	ret = mt6337_config_interface(0x9434, 0x0, 0x1, 15);
	/* [5:3]: AUXADC_AVG_NUM_LARGE; Jyun-Jia/256 average */
	ret = mt6337_config_interface(0x943A, 0x7, 0x7, 3);
	/* [7:6]: AUXADC_TRIM_CH3_SEL; Jyun-Jia/ efuse ch0 */
	ret = mt6337_config_interface(0x9444, 0x0, 0x3, 6);
	/* [9:8]: AUXADC_TRIM_CH4_SEL; Jyun-Jia/ efuse ch4 */
	ret = mt6337_config_interface(0x9444, 0x1, 0x3, 8);
	/* [11:10]: AUXADC_TRIM_CH5_SEL; Jyun-Jia/ efuse ch7 */
	ret = mt6337_config_interface(0x9444, 0x2, 0x3, 10);
	/* [13:12]: AUXADC_TRIM_CH6_SEL; Jyun-Jia/ efuse ch0 */
	ret = mt6337_config_interface(0x9444, 0x0, 0x3, 12);
	/* [15:14]: AUXADC_TRIM_CH7_SEL; Jyun-Jia/ efuse ch7 */
	ret = mt6337_config_interface(0x9444, 0x2, 0x3, 14);
	/* [3:2]: AUXADC_TRIM_CH9_SEL; Jyun-Jia/ efuse ch7 */
	ret = mt6337_config_interface(0x9446, 0x2, 0x3, 2);
	/* [9:8]: AUXADC_TRIM_CH12_SEL; Jyun-Jia/ efuse ch7 */
	ret = mt6337_config_interface(0x9446, 0x2, 0x3, 8);
	/* [14:14]: AUXADC_START_SHADE_EN; Jyun-Jia */
	ret = mt6337_config_interface(0x9456, 0x1, 0x1, 14);
	/* [10:0]: GPIO_PULLEN0; Joseph/ no pull */
	ret = mt6337_config_interface(0x9C06, 0x0, 0x7FF, 0);
	/* [11:9]: GPIO8_MODE; Joseph/ MTKIF */
	ret = mt6337_config_interface(0x9C28, 0x1, 0x7, 9);
	/* [14:12]: GPIO9_MODE; Joseph/ MTKIF */
	ret = mt6337_config_interface(0x9C28, 0x1, 0x7, 12);
	pr_err(MT6337TAG "[%s] 2016-10-13 initial setting Done\n", __func__);
}

static int mt6337_probe(struct platform_device *dev)
{
	int ret_device_file = 0;

	MT6337LOG("******** MT6337 driver probe!! ********\n");
	/*get PMIC CID */
	pr_err("MT6337 CID=0x%x", mt6337_get_register_value(MT6337_PMIC_SWCID));

	/* mt6337_upmu_set_reg_value(0x2a6, 0xff); */ /* TBD */

	/* MT6337 initial setting */
	MT6337_INIT_SETTING();

#if 0
	MT6337LOG("[PMIC_EINT_SETTING] disable when CONFIG_FPGA_EARLY_PORTING\n");
#else
	/* MT6337 Interrupt Service */
	MT6337_EINT_SETTING();
	MT6337LOG("[MT6337_EINT_SETTING] Done\n");
#endif

	mt6337_debug_init(dev);
	MT6337LOG("[MT6337] mt6337_debug_init : done.\n");

	ret_device_file = device_create_file(&(dev->dev), &dev_attr_mt6337_access);
	ret_device_file = device_create_file(&(dev->dev), &dev_attr_mt6337_auxadc_ut);

	MT6337LOG("[PMIC] device_create_file for EM : done.\n");

	/*pwrkey_sw_workaround_init(); */
	/* To prevent from writing Main PMIC when accessing MT6337, need this setting */
	mt6337_set_register_value(MT6337_PMIC_RG_REG_CK_PDN_HWEN, 0x0);
	return 0;
}

static int mt6337_remove(struct platform_device *dev)
{
	MT6337LOG("******** MT6337 pmic driver remove!! ********\n");

	return 0;
}

static void mt6337_shutdown(struct platform_device *dev)
{
	MT6337LOG("******** MT6337 pmic driver shutdown!! ********\n");
}

static int mt6337_suspend(struct platform_device *dev, pm_message_t state)
{
	pmic_suspend_state_mt6337 = true;

	MT6337LOG("******** MT6337 pmic driver suspend!! ********\n");

	return 0;
}

static int mt6337_resume(struct platform_device *dev)
{
	pmic_suspend_state_mt6337 = false;

	MT6337LOG("******** MT6337 driver resume!! ********\n");

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

	pmic_init_wake_lock(&pmicThread_lock_mt6337, "pmicThread_lock_mt6337 wakelock");
	/* PMIC device driver register*/
	ret = platform_device_register(&mt6337_device);
	if (ret) {
		pr_err("****[%s] Unable to device register(%d)\n", __func__, ret);
		return ret;
	}
	ret = platform_driver_register(&mt6337_driver);
	if (ret) {
		pr_err("****[%s] Unable to register driver (%d)\n", __func__, ret);
		return ret;
	}

	MT6337LOG("****[%s] Initialization : DONE !!\n", __func__);

	return 0;
}

static void __exit mt6337_exit(void)
{
	platform_driver_unregister(&mt6337_driver);
}

module_init(mt6337_init);
module_exit(mt6337_exit);

MODULE_AUTHOR("Jimmy-YJ Huang");
MODULE_DESCRIPTION("MT6337 PMIC Device Driver");
MODULE_LICENSE("GPL");

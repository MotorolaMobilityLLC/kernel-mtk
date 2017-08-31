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
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <mt-plat/upmu_common.h>
#include "include/pmic.h"
#include "include/pmic_debugfs.h"

/*-------pmic_dbg_level global variable-------*/
unsigned int gPMICDbgLvl;


/*
 * PMIC dump exception status
 */
/* Kernel dump log */
void kernel_dump_exception_reg(void)
{
	/* 1.UVLO off */
	kernel_output_reg(MT6335_TOP_RST_STATUS);
	kernel_output_reg(MT6335_PONSTS);
	kernel_output_reg(MT6335_POFFSTS);
	/* 2.thermal shutdown 150 */
	kernel_output_reg(MT6335_THERMALSTATUS);
	/* 3.power not good */
	kernel_output_reg(MT6335_PGSTATUS0);
	kernel_output_reg(MT6335_PGSTATUS1);
	/* 4.BUCK OC */
	kernel_output_reg(MT6335_PSOCSTATUS);
	kernel_output_reg(MT6335_BUCK_OC_CON0);
	kernel_output_reg(MT6335_BUCK_OC_CON1);
	/* 5.long press shutdown */
	kernel_output_reg(MT6335_STRUP_CON4);
	/* 6.WDTRST */
	kernel_output_reg(MT6335_TOP_RST_MISC);
	/* 7.CLK TRIM */
	kernel_output_reg(MT6335_TOP_CLK_TRIM);
}
/* Kernel & UART dump log */
void both_dump_exception_reg(struct seq_file *s)
{
	/* 1.UVLO off */
	both_output_reg(MT6335_TOP_RST_STATUS);
	both_output_reg(MT6335_PONSTS);
	both_output_reg(MT6335_POFFSTS);
	/* 2.thermal shutdown 150 */
	both_output_reg(MT6335_THERMALSTATUS);
	/* 3.power not good */
	both_output_reg(MT6335_PGSTATUS0);
	both_output_reg(MT6335_PGSTATUS1);
	/* 4.BUCK OC */
	both_output_reg(MT6335_PSOCSTATUS);
	both_output_reg(MT6335_BUCK_OC_CON0);
	both_output_reg(MT6335_BUCK_OC_CON1);
	/* 5.long press shutdown */
	both_output_reg(MT6335_STRUP_CON4);
	/* 6.WDTRST */
	both_output_reg(MT6335_TOP_RST_MISC);
	/* 7.CLK TRIM */
	both_output_reg(MT6335_TOP_CLK_TRIM);
}

static int pmic_dump_exception_show(struct seq_file *s, void *unused)
{
	both_dump_exception_reg(s);
	return 0;
}
static int pmic_dump_exception_open(struct inode *inode, struct file *file)
{
	return single_open(file, pmic_dump_exception_show, NULL);
}
const struct file_operations pmic_dump_exception_operations = {
	.open    = pmic_dump_exception_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

/*
 * PMIC debug level
 */
/*-------set function-------*/
static unsigned int pmic_dbg_level_set(unsigned int level)
{
	gPMICDbgLvl = level > PMIC_LOG_DBG ? PMIC_LOG_DBG : level;
	return 0;
}

/*-------pmic_dbg_level-------*/
static ssize_t pmic_dbg_level_write(struct file *file, const char __user *buf, size_t size, loff_t *ppos)
{
	char info[10];
	int value = 0;

	memset(info, 0, 10);

	if (copy_from_user(info, buf, size))
		return -EFAULT;

	if ((info[0] >= '0') && (info[0] <= '9'))
		value = (info[0] - 48);

	if (value < 5) {
		pmic_dbg_level_set(value);
		pr_err("pmic_dbg_level_write = %d\n", gPMICDbgLvl);
	} else {
#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
		pmic_ipi_test_code();
		pr_err("pmic_ipi_test_code\n");
#else
		pr_err("pmic_dbg_level should < 5\n");
#endif
	}

	return size;
}

static int pmic_dbg_level_show(struct seq_file *s, void *unused)
{
	seq_puts(s, "4:PMIC_LOG_DBG\n");
	seq_puts(s, "3:PMIC_LOG_INFO\n");
	seq_puts(s, "2:PMIC_LOG_NOT\n");
	seq_puts(s, "1:PMIC_LOG_WARN\n");
	seq_puts(s, "0:PMIC_LOG_ERR\n");
	seq_printf(s, "PMIC_Dbg_Lvl = %d\n", gPMICDbgLvl);
	return 0;
}

static int pmic_dbg_level_open(struct inode *inode, struct file *file)
{
	return single_open(file, pmic_dbg_level_show, NULL);
}

static const struct file_operations pmic_dbg_level_operations = {
	.open    = pmic_dbg_level_open,
	.read    = seq_read,
	.write   = pmic_dbg_level_write,
	.llseek  = seq_lseek,
	.release = single_release,
};

static int __init pmic_debugfs_init(void)
{
	if (!mtk_pmic_dir) {
		pr_err(PMICTAG "dir mtk_pmic does not exist\n");
		return -1;
	}

	debugfs_create_file("pmic_dump_exception", (S_IFREG | S_IRUGO),
		mtk_pmic_dir, NULL, &pmic_dump_exception_operations);
	debugfs_create_file("pmic_dbg_level", (S_IFREG | S_IRUGO),
		mtk_pmic_dir, NULL, &pmic_dbg_level_operations);

	return 0;
}
late_initcall(pmic_debugfs_init);
/*----------------pmic log debug machanism-----------------------*/

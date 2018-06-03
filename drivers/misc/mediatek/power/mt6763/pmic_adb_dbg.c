/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <generated/autoconf.h>
#include <linux/module.h>
/*----------------debug machanism-----------------------*/
#include <linux/uaccess.h>
#include <linux/debugfs.h>
/*----------------debug machanism-----------------------*/
#include <mt-plat/upmu_common.h>
#include "pmic_adb_dbg.h"

void kernel_dump_exception_reg(void)
{
	/* 1.UVLO off */
	kernel_output_reg(MT6351_TOP_RST_STATUS);
	/* 2.thermal shutdown 150 */
	kernel_output_reg(MT6351_THERMALSTATUS);
	/* 3.power not good */
	kernel_output_reg(MT6351_PGSTATUS0);
	kernel_output_reg(MT6351_PGSTATUS1);
	/* 4.LDO OC */
	kernel_output_reg(MT6351_OCSTATUS1);
	kernel_output_reg(MT6351_OCSTATUS2);
	/* 5.long press shutdown */
	kernel_output_reg(MT6351_STRUP_CON12);
	/* 6.WDTRST */
	kernel_output_reg(MT6351_TOP_RST_MISC);
	kernel_output_reg(MT6351_TOP_CLK_TRIM);
	/* 7. BUCK OC */
	kernel_output_reg(MT6351_BUCK_OC_CON0);
	kernel_output_reg(MT6351_BUCK_OC_CON1);
	/* 8.Additional OC Shutdown Information */
	kernel_output_reg(MT6351_BUCK_OC_CON2);
	kernel_output_reg(MT6351_STRUP_CON9);
	kernel_output_reg(MT6351_STRUP_CON6);
	kernel_output_reg(MT6351_STRUP_CON7);
	kernel_output_reg(MT6351_PGDEBSTATUS0);
	kernel_output_reg(MT6351_PGDEBSTATU1);
	kernel_output_reg(MT6351_LDO_VCAMD_CON1);
	kernel_output_reg(MT6351_LDO_VSRAM_PROC_CON1);
	kernel_output_reg(MT6351_LDO_VRF12_CON1);
	kernel_output_reg(MT6351_LDO_VA10_CON1);
	kernel_output_reg(MT6351_LDO_VDRAM_CON1);
	kernel_output_reg(MT6351_STRUP_CON4);
	/* Thermal IRQ status */
	kernel_output_reg(MT6351_STRUP_CON21);
}

/* Kernel & UART dump log */
void both_dump_exception_reg(struct seq_file *s)
{
	/* 1.UVLO off */
	both_output_reg(MT6351_TOP_RST_STATUS);
	/* 2.thermal shutdown 150 */
	both_output_reg(MT6351_THERMALSTATUS);
	/* 3.power not good */
	both_output_reg(MT6351_PGSTATUS0);
	both_output_reg(MT6351_PGSTATUS1);
	/* 4.LDO OC */
	both_output_reg(MT6351_OCSTATUS1);
	both_output_reg(MT6351_OCSTATUS2);
	/* 5.long press shutdown */
	both_output_reg(MT6351_STRUP_CON12);
	/* 6.WDTRST */
	both_output_reg(MT6351_TOP_RST_MISC);
	both_output_reg(MT6351_TOP_CLK_TRIM);
	/* 7. BUCK OC */
	both_output_reg(MT6351_BUCK_OC_CON0);
	both_output_reg(MT6351_BUCK_OC_CON1);
	/* 8.Additional OC Shutdown Information */
	both_output_reg(MT6351_BUCK_OC_CON2);
	both_output_reg(MT6351_STRUP_CON9);
	both_output_reg(MT6351_STRUP_CON6);
	both_output_reg(MT6351_STRUP_CON7);
	both_output_reg(MT6351_PGDEBSTATU1);
	both_output_reg(MT6351_LDO_VCAMD_CON1);
	both_output_reg(MT6351_LDO_VSRAM_PROC_CON1);
	both_output_reg(MT6351_LDO_VRF12_CON1);
	both_output_reg(MT6351_LDO_VA10_CON1);
	both_output_reg(MT6351_LDO_VDRAM_CON1);
	both_output_reg(MT6351_STRUP_CON4);
	/* Thermal IRQ status */
	both_output_reg(MT6351_STRUP_CON21);
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

MODULE_AUTHOR("Jeter Chen");
MODULE_DESCRIPTION("MT PMIC Device Driver");
MODULE_LICENSE("GPL");

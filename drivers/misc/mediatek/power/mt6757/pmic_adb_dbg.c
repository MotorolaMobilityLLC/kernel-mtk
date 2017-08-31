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

#define output(reg) \
	do { \
		seq_printf(s, "[PMIC Exception] " #reg " Reg[0x%x]=0x%x\n", \
			reg, upmu_get_reg_value(reg)); \
		pr_err("[PMIC Exception] " #reg " Reg[0x%x]=0x%x\n", \
			reg, upmu_get_reg_value(reg)); \
	} while (0)

/* pmic_dump_exception */
#if 0
static void output_register(struct seq_file *s, unsigned int reg)
{
	seq_printf(s, "[PMIC Exception] Reg[0x%x]=0x%x\n", reg, upmu_get_reg_value(reg));
	pr_err("[PMIC Exception] Reg[0x%x]=0x%x\n", reg, upmu_get_reg_value(reg));
}
#endif
static int pmic_dump_exception_show(struct seq_file *s, void *unused)
{
	output(MT6351_TOP_RST_STATUS);
	output(MT6351_THERMALSTATUS);
	output(MT6351_PGSTATUS0);
	output(MT6351_PGSTATUS1);
	output(MT6351_OCSTATUS1);
	output(MT6351_OCSTATUS2);
	output(MT6351_STRUP_CON12);
	output(MT6351_TOP_RST_MISC);
	output(MT6351_TOP_CLK_TRIM);
	output(MT6351_BUCK_OC_CON0);
	output(MT6351_BUCK_OC_CON1);
	output(MT6351_BUCK_OC_CON2);
	output(MT6351_STRUP_CON9);
	output(MT6351_STRUP_CON6);
	output(MT6351_STRUP_CON7);
	output(MT6351_PGDEBSTATU1);
	output(MT6351_LDO_VCAMD_CON1);
	output(MT6351_LDO_VSRAM_PROC_CON1);
	output(MT6351_LDO_VRF12_CON1);
	output(MT6351_LDO_VA10_CON1);
	output(MT6351_LDO_VDRAM_CON1);
	output(MT6351_STRUP_CON4);
#if 0
	output_register(s, MT6351_TOP_RST_STATUS);
	output_register(s, MT6351_THERMALSTATUS);
	output_register(s, MT6351_PGSTATUS0);
	output_register(s, MT6351_PGSTATUS1);
	output_register(s, MT6351_OCSTATUS1);
	output_register(s, MT6351_OCSTATUS2);
	output_register(s, MT6351_STRUP_CON12);
	output_register(s, MT6351_TOP_RST_MISC);
	output_register(s, MT6351_TOP_CLK_TRIM);
	output_register(s, MT6351_BUCK_OC_CON0);
	output_register(s, MT6351_BUCK_OC_CON1);
	output_register(s, MT6351_BUCK_OC_CON2);
	output_register(s, MT6351_STRUP_CON9);
	output_register(s, MT6351_STRUP_CON6);
	output_register(s, MT6351_STRUP_CON7);
	output_register(s, MT6351_PGDEBSTATU1);
	output_register(s, MT6351_LDO_VCAMD_CON1);
	output_register(s, MT6351_LDO_VSRAM_PROC_CON1);
	output_register(s, MT6351_LDO_VRF12_CON1);
	output_register(s, MT6351_LDO_VA10_CON1);
	output_register(s, MT6351_LDO_VDRAM_CON1);
	output_register(s, MT6351_STRUP_CON4);
#endif
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

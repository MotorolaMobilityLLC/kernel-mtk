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

#ifndef __PMIC_DEBUGFS_H__
#define __PMIC_DEBUGFS_H__

#include <linux/dcache.h>

#define adb_output_reg(reg) \
	seq_printf(s, "[pmic_boot_status] " #reg " Reg[0x%x]=0x%x\n", reg, upmu_get_reg_value(reg))
#define kernel_output_reg(reg) \
	pr_err("[pmic_boot_status] " #reg " Reg[0x%x]=0x%x\n", reg, upmu_get_reg_value(reg))
#define both_output_reg(reg) \
	do { \
		seq_printf(s, "[pmic_boot_status] " #reg " Reg[0x%x]=0x%x\n", \
			reg, upmu_get_reg_value(reg)); \
		pr_err("[pmic_boot_status] " #reg " Reg[0x%x]=0x%x\n", \
			reg, upmu_get_reg_value(reg)); \
	} while (0)

#define PMIC_LOG_DBG     4
#define PMIC_LOG_INFO    3
#define PMIC_LOG_NOT     2
#define PMIC_LOG_WARN    1
#define PMIC_LOG_ERR     0

/* extern variable */
extern struct dentry *mtk_pmic_dir;
extern unsigned int gPMICDbgLvl;

/* extern function */
extern void kernel_dump_exception_reg(void);

#endif	/* __PMIC_DEBUGFS_H__ */

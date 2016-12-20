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

#ifndef __PMIC_ADB_DBG_H
#define __PMIC_ADB_DBG_H

#define adb_output_reg(reg) \
	seq_printf(s, "[PMIC Exception] " #reg " Reg[0x%x]=0x%x\n", reg, upmu_get_reg_value(reg))

#define kernel_output_reg(reg) \
	pr_err("[PMIC Exception] " #reg " Reg[0x%x]=0x%x\n", reg, upmu_get_reg_value(reg))

#define both_output_reg(reg) \
	do { \
		seq_printf(s, "[PMIC Exception] " #reg " Reg[0x%x]=0x%x\n", \
			reg, upmu_get_reg_value(reg)); \
		pr_err("[PMIC Exception] " #reg " Reg[0x%x]=0x%x\n", \
			reg, upmu_get_reg_value(reg)); \
	} while (0)


extern const struct file_operations pmic_dump_exception_operations;

extern void kernel_dump_exception_reg(void);
#endif /*--PMIC_ADB_DBG_H--*/

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

#ifndef _MT6337_PMIC_H_
#define _MT6337_PMIC_H_

#include "mtk_pmic_common.h"
#include "pmic_debugfs.h"
#include "mt6337_upmu_hw.h"
#include "mt6337_irq.h"

/* The CHIP INFO */
#define PMIC6337_E1_CID_CODE    0x3710
#define PMIC6337_E2_CID_CODE    0x3720
#define PMIC6337_E3_CID_CODE    0x3730

#define MT6337TAG                "[MT6337] "

#define MT6337LOG(fmt, arg...) do { \
	if (gPMICDbgLvl >= PMIC_LOG_DBG) \
		pr_notice(MT6337TAG "%s: " fmt, __func__, ##arg); \
} while (0)

extern const MT6337_PMU_FLAG_TABLE_ENTRY mt6337_pmu_flags_table[];

extern unsigned int mt6337_read_interface(unsigned int RegNum, unsigned int *val,
	unsigned int MASK, unsigned int SHIFT);
extern unsigned int mt6337_config_interface(unsigned int RegNum, unsigned int val,
	unsigned int MASK, unsigned int SHIFT);

extern unsigned int mt6337_config_interface_nolock(unsigned int RegNum, unsigned int val,
	unsigned int MASK, unsigned int SHIFT);

extern unsigned int mt6337_upmu_get_reg_value(unsigned int reg);
extern unsigned int mt6337_upmu_set_reg_value(unsigned int reg, unsigned int reg_val);

extern unsigned short mt6337_set_register_value(MT6337_PMU_FLAGS_LIST_ENUM flagname, unsigned int val);
extern int mt6337_get_register_value(MT6337_PMU_FLAGS_LIST_ENUM flagname);
#endif				/* _MT6337_PMIC_H_ */

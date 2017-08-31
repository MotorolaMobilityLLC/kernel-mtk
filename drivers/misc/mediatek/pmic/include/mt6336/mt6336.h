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

#ifndef _MT_PMIC_6336_H_
#define _MT_PMIC_6336_H_

#include "../../include/mtk_pmic_common.h"
#include "../../include/pmic_debugfs.h"
#include "mt6336_irq.h"
#ifndef MT6336_E1
#include "mt6336_upmu_hw.h"
#else
#include "mt6336_upmu_hw_e1.h"
#endif

/*
 * Debugfs
 */
#define MT6336TAG                "[MT6336] "

#define MT6336LOG(fmt, arg...) do { \
	if (gPMICDbgLvl >= PMIC_LOG_DBG) \
		pr_notice(MT6336TAG "%s: " fmt, __func__, ##arg); \
} while (0)

/* MT6336 controller */
struct mt6336_ctrl {
	const char *name;
	unsigned int id;
	unsigned int state;
};
extern struct mt6336_ctrl *mt6336_ctrl_get(const char *name);
extern unsigned int mt6336_ctrl_enable(struct mt6336_ctrl *ctrl);
extern unsigned int mt6336_ctrl_disable(struct mt6336_ctrl *ctrl);

/* multi-read register */
extern unsigned int mt6336_read_bytes(unsigned int reg, unsigned char *returnData, unsigned int len);
extern unsigned int mt6336_write_bytes(unsigned int, unsigned char *, unsigned int);

/* access register api */
extern unsigned int mt6336_read_interface(unsigned int RegNum, unsigned char *val, unsigned char MASK,
unsigned char SHIFT);
extern unsigned int mt6336_config_interface(unsigned int RegNum, unsigned char val, unsigned char MASK,
unsigned char SHIFT);

/* write one register directly */
extern unsigned int mt6336_set_register_value(unsigned int RegNum, unsigned char val);
extern unsigned int mt6336_get_register_value(unsigned int RegNum);

/* write one register flag */
extern unsigned short mt6336_set_flag_register_value(MT6336_PMU_FLAGS_LIST_ENUM flagname, unsigned char val);
extern unsigned short mt6336_get_flag_register_value(MT6336_PMU_FLAGS_LIST_ENUM flagname);

extern const MT6336_PMU_FLAG_TABLE_ENTRY mt6336_pmu_flags_table[MT6336_PMU_COMMAND_MAX];
#endif /* _MT_PMIC_6336_H_ */

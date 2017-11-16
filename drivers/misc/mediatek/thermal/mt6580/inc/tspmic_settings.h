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

#include <mach/upmu_hw.h>
#include <mach/upmu_sw.h>
/* #include <mach/mt_pmic_wrap.h> */

#define mtktspmic_TEMP_CRIT 150000	/* 150.000 degree Celsius */
#define y_pmic_repeat_times	1

#define mtktspmic_info(fmt, args...) pr_debug("Power/PMIC_Thermal" fmt, ##args)

#define mtktspmic_dprintk(fmt, args...)   \
do {									\
	if (mtktspmic_debug_log) {				\
		pr_debug("Power/PMIC_Thermal" fmt, ##args); \
	}							   \
} while (0)

extern int mtktspmic_debug_log;
extern void mtktspmic_cali_prepare(void);
extern void mtktspmic_cali_prepare2(void);
extern int mtktspmic_get_hw_temp(void);
extern u32 pmic_Read_Efuse_HPOffset(int i);

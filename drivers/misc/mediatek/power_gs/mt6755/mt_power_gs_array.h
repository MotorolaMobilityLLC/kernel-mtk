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

#if defined CONFIG_MTK_PMIC_CHIP_MT6353

extern const unsigned int *MT6353_PMIC_REG_gs_flightmode_suspend_mode;
extern unsigned int MT6353_PMIC_REG_gs_flightmode_suspend_mode_len;
extern const unsigned int *MT6353_PMIC_REG_gs_early_suspend_deep_idle_mode;
extern unsigned int MT6353_PMIC_REG_gs_early_suspend_deep_idle_mode_len;

#else

extern const unsigned int *MT6351_PMIC_REG_gs_flightmode_suspend_mode;
extern unsigned int MT6351_PMIC_REG_gs_flightmode_suspend_mode_len;
extern const unsigned int *MT6351_PMIC_REG_gs_early_suspend_deep_idle_mode;
extern unsigned int MT6351_PMIC_REG_gs_early_suspend_deep_idle_mode_len;

#endif

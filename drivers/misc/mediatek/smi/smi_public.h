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

#ifndef __SMI_PUBLIC_H__
#define __SMI_PUBLIC_H__

#include <mt-plat/mtk_smi.h>
#include "smi_configuration.h"

#ifdef CONFIG_MTK_SMI_EXT
int smi_bus_prepare_enable(const unsigned int reg_indx,
	const char *user_name, const bool mtcmos);
int smi_bus_disable_unprepare(const unsigned int reg_indx,
	const char *user_name, const bool mtcmos);
enum MTK_SMI_BWC_SCEN smi_get_current_profile(void);
void smi_common_ostd_setting(int enable);

#else
#define smi_bus_prepare_enable(reg_indx, user_name, mtcmos) ((void)0)
#define smi_bus_disable_unprepare(reg_indx, user_name, mtcmos) ((void)0)
#define smi_get_current_profile() ((void)0)
#define smi_common_ostd_setting(enable) ((void)0)
#endif

#endif

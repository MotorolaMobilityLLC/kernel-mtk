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

#include "mtk_smi.h"
#include "smi_clk.h"


#if !defined(CONFIG_MTK_SMI_EXT)
#define smi_get_current_profile() ((void)0)
#define smi_bus_enable(master_id, user_name) ((void)0)
#define smi_bus_disable(master_id, user_name) ((void)0)
#define smi_clk_prepare(master_id, user_name, enable_mtcmos) ((void)0)
#define smi_clk_enable(master_id, user_name, enable_mtcmos) ((void)0)
#define smi_clk_unprepare(master_id, user_name, enable_mtcmos) ((void)0)
#define smi_clk_disable(master_id, user_name, enable_mtcmos) ((void)0)
#else
/* SMI extern API */
extern MTK_SMI_BWC_SCEN smi_get_current_profile(void);
extern int smi_bus_enable(enum SMI_MASTER_ID master_id, char *user_name);
extern int smi_bus_disable(enum SMI_MASTER_ID master_id, char *user_name);
extern int smi_clk_prepare(enum SMI_MASTER_ID master_id, char *user_name, int enable_mtcmos);
extern int smi_clk_enable(enum SMI_MASTER_ID master_id, char *user_name, int enable_mtcmos);
extern int smi_clk_unprepare(enum SMI_MASTER_ID master_id, char *user_name, int enable_mtcmos);
extern int smi_clk_disable(enum SMI_MASTER_ID master_id, char *user_name, int enable_mtcmos);
extern void smi_common_ostd_setting(int enable);
extern unsigned long get_larb_base_addr(int larb_id);
extern unsigned long get_common_base_addr(void);
#endif
#endif

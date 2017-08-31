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

#ifndef __MTK_STATIC_POWER_MTK6799_H__
#define __MTK_STATIC_POWER_MTK6799_H__

#define V_OF_FUSE		1100
#define T_OF_FUSE		30
#define FAB_INFO6		19
#define FAB_INFO7		99

#define DEF_BIG_LEAKAGE		58
#define DEF_CPULL_LEAKAGE	27
#define DEF_CPUL_LEAKAGE	45
#define DEF_CCI_LEAKAGE		5
#define DEF_GPU_LEAKAGE		93
#define DEF_VCORE_LEAKAGE	144
#define DEF_VMD1_LEAKAGE	39
#define DEF_MODEM_LEAKAGE	41

enum {
	MTK_SPOWER_CPUBIG = 0,
	MTK_SPOWER_CPUBIG_CLUSTER,
	MTK_SPOWER_CPULL,
	MTK_SPOWER_CPULL_CLUSTER,
	MTK_SPOWER_CPUL,
	MTK_SPOWER_CPUL_CLUSTER,
	MTK_SPOWER_CPU_MAX,
	MTK_SPOWER_CCI,
	MTK_SPOWER_GPU,
	MTK_SPOWER_VCORE,
	MTK_SPOWER_VMD1,
	MTK_SPOWER_MODEM,
	MTK_SPOWER_MAX
};

enum {
	MTK_BIG_LEAKAGE = 0,
	MTK_LL_LEAKAGE,
	MTK_L_LEAKAGE,
	MTK_CCI_LEAKAGE,
	MTK_GPU_LEAKAGE,
	MTK_VCORE_LEAKAGE,
	MTK_MD1_LEAKAGE,
	MTK_MODEM_LEAKAGE,
	MTK_LEAKAGE_MAX
};

extern char *spower_name[];
extern char *leakage_name[];
extern int default_leakage[];
extern int devinfo_idx[];
extern int devinfo_offset[];
extern int devinfo_table[];
#endif

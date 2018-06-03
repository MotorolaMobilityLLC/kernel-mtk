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

#ifndef __MTK_STATIC_POWER_MTK6763_H__
#define __MTK_STATIC_POWER_MTK6763_H__

/* todo: check V_OF_FUSE, T_OF_FUSE */
#define V_OF_FUSE		1100
#define T_OF_FUSE		30

/* devinfo offset for each bank */
#define DEVINFO_IDX_LL 52 /* 0588 */
#define DEVINFO_IDX_L 54 /* 00590 */
#define DEVINFO_IDX_CCI 56 /* 0598 */
#define DEVINFO_IDX_GPU 58 /* 05A0 */
#define DEVINFO_IDX_VCORE 60 /* 05A8 */
#define DEVINFO_IDX_VMD 62 /* 05B0 */
#define DEVINFO_IDX_MODEM 64 /* 05B8 */

#define DEVINFO_OFF_LL 8
#define DEVINFO_OFF_L 8
#define DEVINFO_OFF_CCI 8
#define DEVINFO_OFF_GPU 8
#define DEVINFO_OFF_VCORE 8
#define DEVINFO_OFF_VMD 8
#define DEVINFO_OFF_MODEM 8

/* todo: fake efuse */
/* default leakage value for each bank */
#define DEF_CPULL_LEAKAGE	27
#define DEF_CPUL_LEAKAGE	34
#define DEF_CCI_LEAKAGE		27
#define DEF_GPU_LEAKAGE		75
#define DEF_VCORE_LEAKAGE	135
#define DEF_VMD_LEAKAGE	135
#define DEF_MODEM_LEAKAGE	135


enum {
	MTK_SPOWER_CPULL,
	MTK_SPOWER_CPULL_CLUSTER,
	MTK_SPOWER_CPUL,
	MTK_SPOWER_CPUL_CLUSTER,
	MTK_SPOWER_CCI,
	MTK_SPOWER_GPU,
	MTK_SPOWER_VCORE,
	MTK_SPOWER_VMD,
	MTK_SPOWER_MODEM,

	MTK_SPOWER_MAX
};

enum {
	MTK_LL_LEAKAGE,
	MTK_L_LEAKAGE,
	MTK_CCI_LEAKAGE,
	MTK_GPU_LEAKAGE,
	MTK_VCORE_LEAKAGE,
	MTK_VMD_LEAKAGE,
	MTK_MODEM_LEAKAGE,

	MTK_LEAKAGE_MAX
};

/* record leakage information that read from efuse */
struct spower_leakage_info {
	const char *name;
	unsigned int devinfo_idx;
	unsigned int devinfo_offset;
	unsigned int value;
};

extern struct spower_leakage_info spower_lkg_info[MTK_SPOWER_MAX];

/* efuse mapping */
#define LL_DEVINFO_DOMAIN (BIT(MTK_SPOWER_CPULL) | BIT(MTK_SPOWER_CPULL_CLUSTER))
#define L_DEVINFO_DOMAIN (BIT(MTK_SPOWER_CPUL) | BIT(MTK_SPOWER_CPUL_CLUSTER))
#define CCI_DEVINFO_DOMAIN (BIT(MTK_SPOWER_CCI))
#define GPU_DEVINFO_DOMAIN (BIT(MTK_SPOWER_GPU))
#define VCORE_DEVINFO_DOMAIN (BIT(MTK_SPOWER_VCORE))
#define VMD_DEVINFO_DOMAIN (BIT(MTK_SPOWER_VMD))
#define MODEM_DEVINFO_DOMAIN (BIT(MTK_SPOWER_MODEM))

/* used to calculate total leakage that search from raw table */
#define BIG_CORE_INSTANCE 2
#define DEFAULT_CORE_INSTANCE 4
#define DEFAULT_INSTANCE 1

extern char *spower_name[];
extern char *leakage_name[];
extern int default_leakage[];
extern int devinfo_idx[];
extern int devinfo_offset[];
extern int devinfo_table[];

#endif

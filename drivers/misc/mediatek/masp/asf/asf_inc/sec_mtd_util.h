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

#ifndef MTD_UTILS_H
#define MTD_UTILS_H

/**************************************************************************
*  PARTITION RECORD
**************************************************************************/
#if defined(CONFIG_MTK_GPT_SCHEME_SUPPORT)

typedef struct _MtdPart {
	char name[16];
	unsigned long long sz;
	unsigned long long off;
	unsigned long long e_size;

} MtdPart;

#else

typedef struct _MtdPart {
	char name[16];
	unsigned int sz;
	unsigned int off;
	unsigned int e_size;

} MtdPart;

#endif
/**************************************************************************
*  MTD CONFIGURATION
**************************************************************************/
/* partition table read from /proc/mtd */
#define MAX_MTD_PARTITIONS              (25)

/* search region and off */
/* work for nand and emmc */
#define ROM_INFO_SEARCH_START           (0x0)

/**************************************************************************
 *  EXPORT VARIABLES
 **************************************************************************/
extern MtdPart mtd_part_map[];

/**************************************************************************
 *  UTILITY
 **************************************************************************/
char *mtd2pl(char *part_name);
char *pl2mtd(char *part_name);

#endif				/* MTD_UTILS_H */

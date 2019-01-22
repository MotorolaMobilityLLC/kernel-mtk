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

#ifndef __MTK_SPOWER_DATA_H_
#define __MTK_SPOWER_DATA_H__

#include "mtk_static_power.h"

typedef struct spower_raw_s {
	int vsize;
	int tsize;
	int table_size;
	int *table[3];
	unsigned int devinfo_domain;
	unsigned int spower_id;
	unsigned int leakage_id;
	unsigned int instance;
} spower_raw_t;

#if defined(CONFIG_MACH_MT6759)
#include "mtk_spower_data_mt6759.h"
#endif

#if defined(CONFIG_MACH_MT6763)
#include "mtk_spower_data_mt6763.h"
#endif

typedef struct voltage_row_s {
	int mV[VSIZE];
} vrow_t;

typedef struct temperature_row_s {
	int deg;
	int mA[VSIZE];
} trow_t;


typedef struct sptab_s {
	int vsize;
	int tsize;
	int *data;	/* array[VSIZE + TSIZE + (VSIZE*TSIZE)] */
	vrow_t *vrow;	/* pointer to voltage row of data */
	trow_t *trow;	/* pointer to temperature row of data */
	unsigned int devinfo_domain;
	unsigned int spower_id;
	unsigned int leakage_id;
	unsigned int instance;
} sptbl_t;

typedef struct sptab_list {
	sptbl_t tab_raw[MAX_TABLE_SIZE];
} sptbl_list;

#define trow(tab, ti)		((tab)->trow[ti])
#define mA(tab, vi, ti)		((tab)->trow[ti].mA[vi])
#define mV(tab, vi)		((tab)->vrow[0].mV[vi])
#define deg(tab, ti)		((tab)->trow[ti].deg)
#define vsize(tab)		((tab)->vsize)
#define tsize(tab)		((tab)->tsize)
#define tab_validate(tab)	(!!(tab) && (tab)->data != NULL)

static inline void spower_tab_construct(sptbl_t *tab, spower_raw_t *raw, unsigned int id)
{
	int i;
	sptbl_t *ptab = (sptbl_t *)tab;

	for (i = 0; i < raw->table_size; i++) {
		ptab->vsize = raw->vsize;
		ptab->tsize = raw->tsize;
		ptab->data = raw->table[i];
		ptab->vrow = (vrow_t *)ptab->data;
		ptab->trow = (trow_t *)(ptab->data + ptab->vsize);
		ptab->devinfo_domain = raw->devinfo_domain;
		ptab->spower_id = id;
		ptab->leakage_id = raw->leakage_id;
		ptab->instance = raw->instance;
		ptab++;
	}
}

#endif


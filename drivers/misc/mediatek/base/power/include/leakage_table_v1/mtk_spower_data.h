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

struct spower_raw_t {
	int vsize;
	int tsize;
	int table_size;
	int *table[3];
};


#if defined(CONFIG_MACH_MT6799)
#include "mtk_spower_data_mt6799.h"
#endif

#if defined(CONFIG_MACH_MT6759)
#include "mtk_spower_data_mt6759.h"
#endif

struct vrow_t {
	int mV[VSIZE];
};

struct trow_t {
	int deg;
	int mA[VSIZE];
};


struct sptbl_t {
	int vsize;
	int tsize;
	int *data;	/* array[VSIZE + TSIZE + (VSIZE*TSIZE)] */
	struct vrow_t *vrow;	/* pointer to voltage row of data */
	struct trow_t *trow;	/* pointer to temperature row of data */
};

#define trow(tab, ti)		((tab)->trow[ti])
#define mA(tab, vi, ti)		((tab)->trow[ti].mA[vi])
#define mV(tab, vi)		((tab)->vrow[0].mV[vi])
#define deg(tab, ti)		((tab)->trow[ti].deg)
#define vsize(tab)		((tab)->vsize)
#define tsize(tab)		((tab)->tsize)
#define tab_validate(tab)	(!!(tab) && (tab)->data != NULL)

static inline void spower_tab_construct(struct sptbl_t (*tab)[], struct spower_raw_t *raw)
{
	int i;
	struct sptbl_t *ptab = (struct sptbl_t *)tab;

	for (i = 0; i < raw->table_size; i++) {
		ptab->vsize = raw->vsize;
		ptab->tsize = raw->tsize;
		ptab->data = raw->table[i];
		ptab->vrow = (struct vrow_t *)ptab->data;
		ptab->trow = (struct trow_t *)(ptab->data + ptab->vsize);
		ptab++;
	}
}

#endif


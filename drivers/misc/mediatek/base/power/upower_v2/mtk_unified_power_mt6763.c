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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/rcupdate.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/export.h>
#include <linux/module.h>
#include <linux/ktime.h>
#include <linux/io.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <mt-plat/mtk_chip.h>

/* local include */
#include "mtk_upower.h"
#include "mtk_unified_power_data.h"
#include "mtk_devinfo.h"
#include "mtk_eem.h"
#include "upmu_common.h"

#ifndef EARLY_PORTING_SPOWER
#include "mtk_common_static_power.h"
#endif

/* #if (NR_UPOWER_TBL_LIST <= 1) */
struct upower_tbl final_upower_tbl[NR_UPOWER_BANK] = {};
/* #endif */

int degree_set[NR_UPOWER_DEGREE] = {
		UPOWER_DEGREE_0,
		UPOWER_DEGREE_1,
		UPOWER_DEGREE_2,
		UPOWER_DEGREE_3,
		UPOWER_DEGREE_4,
		UPOWER_DEGREE_5,
};

/* collect all the raw tables */
#define INIT_UPOWER_TBL_INFOS(name, tbl) {__stringify(name), &tbl}
struct upower_tbl_info upower_tbl_infos_list[NR_UPOWER_TBL_LIST][NR_UPOWER_BANK] = {
	[0] = {
		INIT_UPOWER_TBL_INFOS(UPOWER_BANK_LL, upower_tbl_ll_1_FY),
		INIT_UPOWER_TBL_INFOS(UPOWER_BANK_L, upower_tbl_l_1_FY),
		INIT_UPOWER_TBL_INFOS(UPOWER_BANK_CLS_LL, upower_tbl_cluster_ll_1_FY),
		INIT_UPOWER_TBL_INFOS(UPOWER_BANK_CLS_L, upower_tbl_cluster_l_1_FY),
		INIT_UPOWER_TBL_INFOS(UPOWER_BANK_CCI, upower_tbl_cci_1_FY),
	},
	[1] = {
		INIT_UPOWER_TBL_INFOS(UPOWER_BANK_LL, upower_tbl_ll_2_FY),
		INIT_UPOWER_TBL_INFOS(UPOWER_BANK_L, upower_tbl_l_2_FY),
		INIT_UPOWER_TBL_INFOS(UPOWER_BANK_CLS_LL, upower_tbl_cluster_ll_2_FY),
		INIT_UPOWER_TBL_INFOS(UPOWER_BANK_CLS_L, upower_tbl_cluster_l_2_FY),
		INIT_UPOWER_TBL_INFOS(UPOWER_BANK_CCI, upower_tbl_cci_2_FY),
	},
	[2] = {
		INIT_UPOWER_TBL_INFOS(UPOWER_BANK_LL, upower_tbl_ll_3_FY),
		INIT_UPOWER_TBL_INFOS(UPOWER_BANK_L, upower_tbl_l_3_FY),
		INIT_UPOWER_TBL_INFOS(UPOWER_BANK_CLS_LL, upower_tbl_cluster_ll_3_FY),
		INIT_UPOWER_TBL_INFOS(UPOWER_BANK_CLS_L, upower_tbl_cluster_l_3_FY),
		INIT_UPOWER_TBL_INFOS(UPOWER_BANK_CCI, upower_tbl_cci_3_FY),
	},
};
/* Upower will know how to apply voltage that comes from EEM */
unsigned char upower_recognize_by_eem[NR_UPOWER_BANK] = {
	UPOWER_BANK_LL, /* LL EEM apply voltage to LL upower bank */
	UPOWER_BANK_L, /* L EEM apply voltage to L upower bank */
	UPOWER_BANK_LL, /* LL EEM apply voltage to CLS_LL upower bank */
	UPOWER_BANK_L, /* L EEM apply voltage to CLS_L upower bank */
	UPOWER_BANK_CCI, /* CCI EEM apply voltage to CCI upower bank */
};

/* Used for rcu lock, points to all the raw tables list*/
struct upower_tbl_info *p_upower_tbl_infos = &upower_tbl_infos_list[0][0];

#ifndef EARLY_PORTING_SPOWER
int upower_bank_to_spower_bank(int upower_bank)
{
	int ret;

	switch (upower_bank) {
	case UPOWER_BANK_LL:
		ret = MTK_SPOWER_CPULL;
		break;
	case UPOWER_BANK_L:
		ret = MTK_SPOWER_CPUL;
		break;
	case UPOWER_BANK_CLS_LL:
		ret = MTK_SPOWER_CPULL_CLUSTER;
		break;
	case UPOWER_BANK_CLS_L:
		ret = MTK_SPOWER_CPUL_CLUSTER;
		break;
	case UPOWER_BANK_CCI:
		ret = MTK_SPOWER_CCI;
		break;
	default:
		ret = -1;
		break;
	}
	return ret;
}
#endif

/****************************************************
 * According to chip version get the raw upower tbl *
 * and let upower_tbl_infos points to it.           *
 * Choose a non used upower tbl location and let    *
 * upower_tbl_ref points to it to store target      *
 * power tbl.                                       *
 ***************************************************/
void get_original_table(void)
{
	unsigned int upower_proj_ver = 0;
	unsigned int binLevel = 0;
	unsigned short idx = 0;
	unsigned int i, j;

	upower_proj_ver = is_ext_buck_exist();
	/* if M17+, idx = 0*/
	if (upower_proj_ver == 1) {
		binLevel = get_devinfo_with_index(UPOWER_FUNC_CODE_EFUSE_INDEX) >> 8 & 0xff;
		if (binLevel == 0)
			idx = 0;
		else if (binLevel == 1)
			idx = 0;
		else if (binLevel == 2)
			idx = 0;
		else
			idx = 0;
	} else {
	/* if M17, idx = 2 */
	/* if M17T, idx = 3 */
		idx = 0;
	}

	upower_error("projver, binLevel, idx=%d, %d, %d\n", upower_proj_ver, binLevel, idx);

	/* get location of reference table */
	upower_tbl_infos = &upower_tbl_infos_list[idx][0];

	/* get location of target table */
/* #if (NR_UPOWER_TBL_LIST <= 1) */
	upower_tbl_ref = &final_upower_tbl[0];
/* #else */
/*	upower_tbl_ref = upower_tbl_infos_list[(idx+1) % NR_UPOWER_TBL_LIST][0].p_upower_tbl; */
/* #endif */

	upower_debug("idx %d dest:%p, src:%p\n",
					(idx+1)%NR_UPOWER_TBL_LIST, upower_tbl_ref, upower_tbl_infos);
	/* p_upower_tbl_infos = upower_tbl_infos; */

#if 0
	upower_debug("upower_tbl_ll_1_FY %p\n", &upower_tbl_ll_1_FY);
	upower_debug("upower_tbl_ll_1_SB %p\n", &upower_tbl_ll_1_SB);
	upower_debug("upower_tbl_ll_2_FY %p\n", &upower_tbl_ll_2_FY);
#endif

	/*
	*  Clear volt fields before eem run.                                  *
	*  If eem is enabled, it will apply volt into it. If eem is disabled, *
	*  the values of volt are 0 , and upower will apply orig volt into it *
	*/
	for (i = 0; i < NR_UPOWER_BANK; i++) {
		for (j = 0; j < UPOWER_OPP_NUM; j++)
			upower_tbl_ref[i].row[j].volt = 0;
	}
	for (i = 0; i < NR_UPOWER_BANK; i++)
		upower_debug("bank[%d] dest:%p dyn_pwr:%u, volt[0]%u\n", i, &upower_tbl_ref[i],
					upower_tbl_ref[i].row[0].dyn_pwr, upower_tbl_ref[i].row[0].volt);
}

MODULE_DESCRIPTION("MediaTek Unified Power Driver v0.0");
MODULE_LICENSE("GPL");

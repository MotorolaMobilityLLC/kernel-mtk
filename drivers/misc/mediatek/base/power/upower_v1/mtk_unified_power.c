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

/* local include */
#include "mtk_unified_power.h"
#include "mtk_unified_power_data.h"
#include "mtk_unified_power_internal.h"
/* #include "mtk_static_power.h" */

#if UPOWER_ENABLE
unsigned char upower_enable = 1;
#else
unsigned char upower_enable;
#endif

/* upower table reference */
struct upower_tbl *upower_tbl_ref;
int degree_set[NR_UPOWER_DEGREE] = {45, 55, 65, 75, 85};
/* collect all the raw tables */
struct upower_tbl_info upower_tbl_infos[NR_UPOWER_BANK] = {
	{.p_upower_tbl = &upower_tbl_ll_FY},
	{.p_upower_tbl = &upower_tbl_l_FY},
	{.p_upower_tbl = &upower_tbl_b_FY},
	{.p_upower_tbl = &upower_tbl_cluster_ll_FY},
	{.p_upower_tbl = &upower_tbl_cluster_l_FY},
	{.p_upower_tbl = &upower_tbl_cluster_b_FY},
	{.p_upower_tbl = &upower_tbl_cci_FY},
};

struct upower_tbl_info upower_tbl_infos_SB[NR_UPOWER_BANK] = {
	{.p_upower_tbl = &upower_tbl_ll_SB},
	{.p_upower_tbl = &upower_tbl_l_SB},
	{.p_upower_tbl = &upower_tbl_b_SB},
	{.p_upower_tbl = &upower_tbl_cluster_ll_SB},
	{.p_upower_tbl = &upower_tbl_cluster_l_SB},
	{.p_upower_tbl = &upower_tbl_cluster_b_SB},
	{.p_upower_tbl = &upower_tbl_cci_SB},
};
/* points to all the raw tables */
struct upower_tbl_info *p_upower_tbl_infos = &upower_tbl_infos[0];

#if 0
static void print_tbl(void)
{
	int i, j;
/* --------------------print static orig table -------------------------*/
	#if 0
	struct upower_tbl *tbl;

	for (i = 0; i < NR_UPOWER_BANK; i++) {
		tbl = upower_tbl_infos[i].p_upower_tbl;
		/* table size must be 512 bytes */
		upower_debug("Bank %d , tbl size %ld\n", i, sizeof(struct upower_tbl));
		for (j = 0; j < OPP_NUM; j++) {
			upower_debug(" cap, volt, dyn, lkg: %llu, %u, %u, {%u, %u, %u, %u, %u}\n",
					tbl->row[j].cap, tbl->row[j].volt,
					tbl->row[j].dyn_pwr, tbl->row[j].lkg_pwr[0],
					tbl->row[j].lkg_pwr[1], tbl->row[j].lkg_pwr[2],
					tbl->row[j].lkg_pwr[3], tbl->row[j].lkg_pwr[4]);
		}

		upower_debug(" lkg_idx, num_row: %d, %d\n", tbl->lkg_idx, tbl->row_num);
		upower_debug("-----------------------------------------------------------------\n");
	}
	#else
/* --------------------print sram table -------------------------*/
	for (i = 0; i < NR_UPOWER_BANK; i++) {
		/* table size must be 512 bytes */
		upower_debug("---Bank %d , tbl size %ld---\n", i, sizeof(struct upower_tbl));
		for (j = 0; j < OPP_NUM; j++) {
			upower_debug(" cap = %llu, volt = %u, dyn = %u, lkg = {%u, %u, %u, %u, %u}\n",
					upower_tbl_ref[i].row[j].cap, upower_tbl_ref[i].row[j].volt,
					upower_tbl_ref[i].row[j].dyn_pwr, upower_tbl_ref[i].row[j].lkg_pwr[0],
					upower_tbl_ref[i].row[j].lkg_pwr[1], upower_tbl_ref[i].row[j].lkg_pwr[2],
					upower_tbl_ref[i].row[j].lkg_pwr[3], upower_tbl_ref[i].row[j].lkg_pwr[4]);
		}
		upower_debug(" lkg_idx, num_row: %d, %d\n",
					upower_tbl_ref[i].lkg_idx, upower_tbl_ref[i].row_num);
		upower_debug("-------------------------------------------------\n");
	}
	#endif
}
#endif

#ifdef UPOWER_UT
void upower_ut(void)
{
	struct upower_tbl_info **addr_ptr_tbl_info;
	struct upower_tbl_info *ptr_tbl_info;
	struct upower_tbl *ptr_tbl;
	int i, j;

	upower_debug("----upower_get_tbl() test----\n");
	/* get addr of ptr which points to upower_tbl_infos[] */
	addr_ptr_tbl_info = upower_get_tbl();
	/* get ptr which points to upower_tbl_infos[] */
	ptr_tbl_info = *addr_ptr_tbl_info;
	upower_debug("ptr_tbl_info --> %p --> tbl %p (p_upower_tbl_infos --> %p)\n",
				ptr_tbl_info, ptr_tbl_info[0].p_upower_tbl, p_upower_tbl_infos);

	/* print all the tables that record in upower_tbl_infos[]*/
	for (i = 0; i < NR_UPOWER_BANK; i++) {
		upower_debug("bank %d\n", i);
		ptr_tbl = ptr_tbl_info[i].p_upower_tbl;
		for (j = 0; j < OPP_NUM; j++) {
			upower_debug(" cap = %llu, volt = %u, dyn = %u, lkg = {%u, %u, %u, %u, %u}\n",
					ptr_tbl->row[j].cap, ptr_tbl->row[j].volt,
					ptr_tbl->row[j].dyn_pwr, ptr_tbl->row[j].lkg_pwr[0],
					ptr_tbl->row[j].lkg_pwr[1], ptr_tbl->row[j].lkg_pwr[2],
					ptr_tbl->row[j].lkg_pwr[3], ptr_tbl->row[j].lkg_pwr[4]);
		}
		upower_debug(" lkg_idx, num_row: %d, %d\n",
					ptr_tbl->lkg_idx, ptr_tbl->row_num);
	}

	upower_debug("----upower_get_power() test----\n");
	for (i = 0; i < NR_UPOWER_BANK; i++) {
		upower_debug("bank %d\n", i);
		upower_debug("[dyn] %u, %u, %u, %u, %u, %u, %u, %u\n",
					upower_get_power(i, 0, UPOWER_DYN),
					upower_get_power(i, 1, UPOWER_DYN),
					upower_get_power(i, 2, UPOWER_DYN),
					upower_get_power(i, 3, UPOWER_DYN),
					upower_get_power(i, 4, UPOWER_DYN),
					upower_get_power(i, 5, UPOWER_DYN),
					upower_get_power(i, 6, UPOWER_DYN),
					upower_get_power(i, 7, UPOWER_DYN));
		upower_debug("[lkg] %u, %u, %u, %u, %u, %u, %u, %u\n",
					upower_get_power(i, 0, UPOWER_LKG),
					upower_get_power(i, 1, UPOWER_LKG),
					upower_get_power(i, 2, UPOWER_LKG),
					upower_get_power(i, 3, UPOWER_LKG),
					upower_get_power(i, 4, UPOWER_LKG),
					upower_get_power(i, 5, UPOWER_LKG),
					upower_get_power(i, 6, UPOWER_LKG),
					upower_get_power(i, 7, UPOWER_LKG));
	}
}
#endif

static void upower_update_dyn_pwr(void)
{
	unsigned long long refPower, newVolt, refVolt, newPower;
	unsigned long long temp1, temp2;
	int i, j;
	struct upower_tbl *tbl;

	for (i = 0; i < NR_UPOWER_BANK; i++) {
		tbl = upower_tbl_infos[i].p_upower_tbl;
		for (j = 0; j < OPP_NUM; j++) {
			refPower = (unsigned long long)tbl->row[j].dyn_pwr;
			refVolt = (unsigned long long)tbl->row[j].volt;
			newVolt = (unsigned long long)upower_tbl_ref[i].row[j].volt;

			temp1 = (refPower * newVolt * newVolt);
			temp2 = (refVolt * refVolt);
			newPower = temp1 / temp2;
			upower_tbl_ref[i].row[j].dyn_pwr = newPower;
			/* upower_debug("dyn_pwr= %u\n", upower_tbl_ref[i].row[j].dyn_pwr);*/
		}
	}
}


#ifndef EARLY_PORTING_SPOWER
static int upower_bank_to_spower_bank(int upower_bank)
{
	int ret;

	switch (upower_bank) {
	case UPOWER_BANK_LL:
		ret = MT_SPOWER_CPULL;
		break;
	case UPOWER_BANK_L:
		ret = MT_SPOWER_CPUL;
		break;
	case UPOWER_BANK_B:
		ret = MT_SPOWER_CPUBIG;
		break;
	case UPOWER_BANK_CLS_LL:
		ret = MT_SPOWER_CPULL_CLUSTER;
		break;
	case UPOWER_BANK_CLS_L:
		ret = MT_SPOWER_CPUL_CLUSTER;
		break;
	case UPOWER_BANK_CLS_B:
		ret = MT_SPOWER_CPUBIG_CLUSTER;
		break;
	case UPOWER_BANK_CCI:
		ret = MT_SPOWER_CCI;
		break;
	default:
		ret = -1;
		break;
	}
	return ret;
}
#endif

static void upower_update_lkg_pwr(void)
{
	int i, j, k;
	#ifdef EARLY_PORTING_SPOWER
	struct upower_tbl *tbl;
	#else
	unsigned int spower_bank_id;
	unsigned int volt;
	int degree;
	#endif

	for (i = 0; i < NR_UPOWER_BANK; i++) {
		#ifdef EARLY_PORTING_SPOWER
		tbl = upower_tbl_infos[i].p_upower_tbl;
		for (j = 0; j < OPP_NUM; j++) {
			for (k = 0; k < NR_UPOWER_DEGREE; k++)
				upower_tbl_ref[i].row[j].lkg_pwr[k] = tbl->row[j].lkg_pwr[k];
		}
		#else
		spower_bank_id = upower_bank_to_spower_bank(i);
		/* wrong bank */
		if (spower_bank_id == -1)
			break;

		for (j = 0; j < OPP_NUM; j++) {
			volt = (unsigned int)upower_tbl_ref[i].row[j].volt;
			for (k = 0; k < NR_UPOWER_DEGREE; k++) {
				degree = degree_set[k];
				/* get leakage from spower driver and transfer mw to uw */
				upower_tbl_ref[i].row[j].lkg_pwr[k] =
				mt_spower_get_leakage(spower_bank_id, (volt/100), degree) * 1000;
			}
#if 0
			upower_debug("volt[%u] deg[%d] lkg_pwr in tbl[%u, %u, %u, %u, %u]\n", volt, degree,
							upower_tbl_ref[i].row[j].lkg_pwr[0],
							upower_tbl_ref[i].row[j].lkg_pwr[1],
							upower_tbl_ref[i].row[j].lkg_pwr[2],
							upower_tbl_ref[i].row[j].lkg_pwr[3],
							upower_tbl_ref[i].row[j].lkg_pwr[4]);
#endif
		}
		#endif
	}
}

static void upower_init_cap(void)
{
	int i, j;
	struct upower_tbl *tbl;

	for (i = 0; i < NR_UPOWER_BANK; i++) {
		tbl = upower_tbl_infos[i].p_upower_tbl;
		for (j = 0; j < OPP_NUM; j++)
			upower_tbl_ref[i].row[j].cap = tbl->row[j].cap;
	}
}

static void upower_init_rownum(void)
{
	int i;

	for (i = 0; i < NR_UPOWER_BANK; i++) {
		upower_tbl_ref[i].row_num = OPP_NUM;
		/*
		*upower_error("[bank %d]lkg_idx=%d, row num = %d\n", i, upower_tbl_ref[i].lkg_idx,
		*								upower_tbl_ref[i].row_num);
		*/
	}
}

#ifdef EARLY_PORTING_EEM
static void upower_init_lkgidx(void)
{
	int i;

	for (i = 0; i < NR_UPOWER_BANK; i++) {
		upower_tbl_ref[i].lkg_idx = NR_UPOWER_DEGREE - 1;
		/*
		*upower_error("[bank %d]lkg_idx=%d, row num = %d\n", i, upower_tbl_ref[i].lkg_idx,
		*								upower_tbl_ref[i].row_num);
		*/
	}
}
static void upower_init_volt(void)
{
	int i, j;
	struct upower_tbl *tbl;

	for (i = 0; i < NR_UPOWER_BANK; i++) {
		tbl = upower_tbl_infos[i].p_upower_tbl;
		for (j = 0; j < OPP_NUM; j++)
			upower_tbl_ref[i].row[j].volt = tbl->row[j].volt;
	}
}
#endif

static int upower_update_tbl_ref(void)
{
	int i;
	struct upower_tbl_info *new_p_tbl_infos;
	int ret = 0;

	#ifdef UPOWER_PROFILE_API_TIME
	upower_get_start_time_us(UPDATE_TBL_PTR);
	#endif

	new_p_tbl_infos = kzalloc(sizeof(new_p_tbl_infos) * NR_UPOWER_BANK, GFP_KERNEL);
	if (!new_p_tbl_infos) {
		upower_error("Out of mem to create new_p_tbl_infos\n");
		return -ENOMEM;
	}

	/* upower_tbl_ref is the ptr points to table in sram */
	for (i = 0; i < NR_UPOWER_BANK; i++)
		new_p_tbl_infos[i].p_upower_tbl = &upower_tbl_ref[i];

	#ifdef UPOWER_RCU_LOCK
	rcu_assign_pointer(p_upower_tbl_infos, new_p_tbl_infos);
	/* synchronize_rcu();*/
	#else
	p_upower_tbl_infos = new_p_tbl_infos;
	#endif

	#ifdef UPOWER_PROFILE_API_TIME
	upower_get_diff_time_us(UPDATE_TBL_PTR);
	print_diff_results(UPDATE_TBL_PTR);
	#endif

	return ret;
}

static int __init upower_get_tbl_ref(void)
{
	/* get table address on sram */
	upower_tbl_ref = ioremap_nocache(UPOWER_TBL_BASE, UPOWER_TBL_TOTAL_SIZE);
	if (upower_tbl_ref == NULL)
		return -ENOMEM;

	upower_error("upower_tbl_ref=%p, limit = %p\n",
					upower_tbl_ref, upower_tbl_ref+UPOWER_TBL_TOTAL_SIZE);

	memset_io((u8 *)upower_tbl_ref, 0x00, UPOWER_TBL_TOTAL_SIZE);

	return 0;
}

#ifdef UPOWER_PROFILE_API_TIME
static void profile_api(void)
{
	int i, j;

	upower_debug("----profile upower_get_power()----\n");
	/* do 56*2 times */
	for (i = 0; i < NR_UPOWER_BANK; i++) {
		for (j = 0; j < OPP_NUM; j++) {
			upower_get_power(i, j, UPOWER_DYN);
			upower_get_power(i, j, UPOWER_LKG);
		}
	}
	upower_debug("----profile upower_update_tbl_ref()----\n");
	for (i = 0; i < 10; i++)
		upower_update_tbl_ref();
}
#endif

static int __init upower_init(void)
{
	if (upower_enable == 0) {
		upower_error("upower is disabled\n");
		return 0;
	}
	/* PTP has no efuse, so volt will be set to orig data */
	/* before upower_init_volt(), PTP has called upower_update_volt_by_eem() */
	upower_debug("upower_tbl_infos[0](%p)= %p\n",
					&upower_tbl_infos[0], upower_tbl_infos[0].p_upower_tbl);

	#ifdef UPOWER_UT
	upower_debug("--------- (UT)before tbl ready--------------\n");
	upower_ut();
	#endif

	/* init rownum to OPP_NUM*/
	upower_init_rownum();
	upower_init_cap();

	#ifdef EARLY_PORTING_EEM
	/* apply orig volt and lkgidx, due to ptp not ready*/
	upower_init_lkgidx();
	upower_init_volt();
	#endif

	upower_update_dyn_pwr();
	upower_update_lkg_pwr();
	upower_update_tbl_ref();

	#ifdef UPOWER_UT
	upower_debug("--------- (UT)tbl ready--------------\n");
	upower_ut();
	#endif

	#ifdef UPOWER_PROFILE_API_TIME
	profile_api();
	#endif
	return 0;
}
#ifdef __KERNEL__
arch_initcall(upower_get_tbl_ref);
late_initcall(upower_init);
#endif
MODULE_DESCRIPTION("MediaTek Unified Power Driver v0.0");
MODULE_LICENSE("GPL");

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

#ifndef MTK_UNIFIED_POWER_H
#define MTK_UNIFIED_POWER_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_MACH_MT6759)
#include "mtk_unified_power_mt6759.h"
#endif

#define UPOWER_TAG "[UPOWER]"

#if UPOWER_LOG
	#define upower_error(fmt, args...) pr_err(UPOWER_TAG fmt, ##args)
	#define upower_debug(fmt, args...) pr_err(UPOWER_TAG fmt, ##args)
#else
	#define upower_error(fmt, args...)
	#define upower_debug(fmt, args...)
#endif

/***************************
 * Basic Data Declarations *
 **************************/
/* 8bytes + 4bytes + 4bytes + 24bytes = 40 bytes*/
/* but compiler will align to 40 bytes for computing more faster */
/* if a table has 16 opps --> 40*16= 640 bytes*/
struct upower_tbl_row {
	unsigned long long cap;
	unsigned int volt; /* 10uv */
	unsigned int dyn_pwr; /* uw */
	unsigned int lkg_pwr[NR_UPOWER_DEGREE]; /* uw */
};

/* struct idle_state defined at sched.h */
/* sizeof(struct upower_tbl) = 5264bytes */
struct upower_tbl {
	struct upower_tbl_row row[UPOWER_OPP_NUM];
	unsigned int lkg_idx;
	unsigned int row_num;
	struct idle_state idle_states[NR_UPOWER_DEGREE][NR_UPOWER_CSTATES];
	unsigned int nr_idle_states;
};

struct upower_tbl_info {
	const char *name;
	struct upower_tbl *p_upower_tbl;
};

/***************************
 * Global variables        *
 **************************/
extern struct upower_tbl *upower_tbl_ref; /* upower table reference to sram*/
extern int degree_set[NR_UPOWER_DEGREE];
extern struct upower_tbl_info *upower_tbl_infos; /* collect all the raw tables */
extern struct upower_tbl_info *p_upower_tbl_infos; /* points to upower_tbl_infos[] */
extern unsigned char upower_enable;
extern struct upower_tbl_info *upower_tbl_infos_list[NR_UPOWER_TBL_LIST];

#if (NR_UPOWER_TBL_LIST <= 1)
extern struct upower_tbl final_upower_tbl[NR_UPOWER_BANK];
#endif

/***************************
 * APIs                    *
 **************************/
/* PPM */
extern unsigned int upower_get_power(enum upower_bank bank, unsigned int opp, enum
upower_dtype type);
/* EAS */
extern struct upower_tbl_info **upower_get_tbl(void);
/* EEM */
extern void upower_update_volt_by_eem(enum upower_bank bank, unsigned int *volt, unsigned int opp_num);
extern void upower_update_degree_by_eem(enum upower_bank bank, int deg);

/* platform part */
extern int upower_bank_to_spower_bank(int upower_bank);
extern void get_original_table(void);

#ifdef UPOWER_RCU_LOCK
extern void upower_read_lock(void);
extern void upower_read_unlock(void);
#endif

#ifdef UPOWER_PROFILE_API_TIME
enum {
	GET_PWR,
	GET_TBL_PTR,
	UPDATE_TBL_PTR,

	TEST_NUM
};
extern void upower_get_start_time_us(unsigned int type);
extern void upower_get_diff_time_us(unsigned int type);
extern void print_diff_results(unsigned int type);
#endif
#ifdef __cplusplus
}
#endif

#endif

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

 /**********************************************
 * unified_power.h
 * This header file includes:
 * 1. Global configs for unified power driver
 * 2. Global macros
 * 3. Declarations of enums and main data structures
 * 4. Extern global variables
 * 5. Extern global APIs
 **********************************************/
#ifndef MTK_UNIFIED_POWER_H
#define MTK_UNIFIED_POWER_H

#ifdef __cplusplus
extern "C" {
#endif

#define UPOWER_ENABLE (1)

#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	#define UPOWER_ENABLE_TINYSYS_SSPM (0)
#else
	#define UPOWER_ENABLE_TINYSYS_SSPM (0)
#endif

#if UPOWER_ENABLE_TINYSYS_SSPM
	#define EARLY_PORTING_EEM /* will restore volt after ptp apply volt */
	#define EARLY_PORTING_SPOWER /* will not get leakage from leakage driver */
	/* #define UPOWER_UT*/
	/* #define UPOWER_PROFILE_API_TIME */
	#define UPOWER_RCU_LOCK
#else
	#define EARLY_PORTING_EEM /* will restore volt after ptp apply volt */
	#define EARLY_PORTING_SPOWER /* will not get leakage from leakage driver */
	/* #define UPOWER_UT */
	/* #define UPOWER_PROFILE_API_TIME */
	#define UPOWER_RCU_LOCK
#endif

#define OPP_NUM 16
#define UPOWER_DEGREE_0 85
#define UPOWER_DEGREE_1 75
#define UPOWER_DEGREE_2 65
#define UPOWER_DEGREE_3 55
#define UPOWER_DEGREE_4 45
#define UPOWER_DEGREE_5 25

#define NR_UPOWER_DEGREE 6
#define DEFAULT_LKG_IDX 0

/* for unified power driver internal use */
#define UPOWER_LOG (1)
#define UPOWER_TAG "[UPOWER]"

#if UPOWER_LOG
	#define upower_error(fmt, args...) pr_err(UPOWER_TAG fmt, ##args)
	#define upower_debug(fmt, args...) pr_debug(UPOWER_TAG fmt, ##args)
#else
	#define upower_error(fmt, args...)
	#define upower_debug(fmt, args...)
#endif

#define PROC_FOPS_RW(name)					\
	static int name ## _proc_open(struct inode *inode,	\
		struct file *file)				\
	{							\
		return single_open(file, name ## _proc_show,	\
			PDE_DATA(inode));			\
	}							\
	static const struct file_operations name ## _proc_fops = {	\
		.owner		  = THIS_MODULE,				\
		.open		   = name ## _proc_open,			\
		.read		   = seq_read,				\
		.llseek		 = seq_lseek,				\
		.release		= single_release,			\
		.write		  = name ## _proc_write,			\
	}

#define PROC_FOPS_RO(name)					\
	static int name ## _proc_open(struct inode *inode,	\
		struct file *file)				\
	{							\
		return single_open(file, name ## _proc_show,	\
			PDE_DATA(inode));			\
	}							\
	static const struct file_operations name ## _proc_fops = {	\
		.owner		  = THIS_MODULE,				\
		.open		   = name ## _proc_open,			\
		.read		   = seq_read,				\
		.llseek		 = seq_lseek,				\
		.release		= single_release,			\
	}

#define PROC_ENTRY(name)	{__stringify(name), &name ## _proc_fops}

/* upower banks */
enum upower_bank {
	UPOWER_BANK_LL,
	UPOWER_BANK_L,
	UPOWER_BANK_B,
	UPOWER_BANK_CLS_LL,
	UPOWER_BANK_CLS_L,
	UPOWER_BANK_CLS_B,
	UPOWER_BANK_CCI,

	NR_UPOWER_BANK,
};

/* for upower_get_power() to get the target power */
enum upower_dtype {
	UPOWER_DYN,
	UPOWER_LKG,

	NR_UPOWER_DTYPE,
};

/***************************
 * Basic Data Declarations *
 **************************/
/* 8bytes + 4bytes + 4bytes + 20bytes = 36 bytes*/
/* but compiler will align to 40 bytes for computing more faster */
/* if a table has 8 opps --> 40*8= 320 bytes*/
struct upower_tbl_row {
	unsigned long long cap;
	unsigned int volt; /* 10uv */
	unsigned int dyn_pwr; /* uw */
	unsigned int lkg_pwr[NR_UPOWER_DEGREE]; /* uw */
};

/* a table : 320 + 4 + 4 = 328 bytes */
/* if we use unsigned char for lkg_idx and row_num */
/* compiler will align unsigned char to unsigned int */
/* hence, we use unsigned int for both params directly */
struct upower_tbl {
	struct upower_tbl_row row[OPP_NUM];
	unsigned int lkg_idx;
	unsigned int row_num;
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
extern struct upower_tbl_info upower_tbl_infos[NR_UPOWER_BANK]; /* collect all the raw tables */
extern struct upower_tbl_info *p_upower_tbl_infos; /* points to upower_tbl_infos[] */
extern unsigned char upower_enable;

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

#ifdef __cplusplus
}
#endif

#endif

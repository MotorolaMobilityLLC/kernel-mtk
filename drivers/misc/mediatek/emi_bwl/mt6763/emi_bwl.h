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

#ifndef __MT_EMI_BW_LIMITER__
#define __MT_EMI_BW_LIMITER__

/*
 * Define EMI hardware registers.
 */

#define EMI_CONA		(CEN_EMI_BASE + 0x0000)
#define EMI_CONM		(CEN_EMI_BASE + 0x0060)
#define EMI_ARBA		(CEN_EMI_BASE + 0x0100)
#define EMI_ARBB		(CEN_EMI_BASE + 0x0108)
#define EMI_ARBC		(CEN_EMI_BASE + 0x0110)
#define EMI_ARBD		(CEN_EMI_BASE + 0x0118)
#define EMI_ARBE		(CEN_EMI_BASE + 0x0120)
#define EMI_ARBF		(CEN_EMI_BASE + 0x0128)
#define EMI_ARBG		(CEN_EMI_BASE + 0x0130)
#define EMI_ARBH		(CEN_EMI_BASE + 0x0138)

#define CHA_EMI_DRS		(CHA_EMI_BASE + 0x0168)
#define CHA_EMI_DRS_ST3		(CHA_EMI_BASE + 0x0180)
#define CHA_EMI_DRS_ST4		(CHA_EMI_BASE + 0x0184)
#define CHA_EMI_DRS_ST5		(CHA_EMI_BASE + 0x0188)

#define CHB_EMI_DRS		(CHB_EMI_BASE + 0x0168)
#define CHB_EMI_DRS_ST3		(CHB_EMI_BASE + 0x0180)
#define CHB_EMI_DRS_ST4		(CHB_EMI_BASE + 0x0184)
#define CHB_EMI_DRS_ST5		(CHB_EMI_BASE + 0x0188)

/*
 * Define constants.
 */
#define MAX_CH	2
#define MAX_RK	2

#define ENABLE_EMI_DRS

/* define supported DRAM types */
enum {
	LPDDR4 = 0,
	mDDR,
};

/* define concurrency scenario ID */
enum {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe, arbf, arbg, arbh, conm) con_sce,
#include "con_sce_lpddr4.h"
#undef X_CON_SCE
	NR_CON_SCE
};

/* define control operation */
enum {
	ENABLE_CON_SCE = 0,
	DISABLE_CON_SCE = 1
};

#define EN_CON_SCE_STR "ON"
#define DIS_CON_SCE_STR "OFF"

/* define DRS HW mask bit */
enum {
	mSPM,
	mRGU,
	mDISP,
	mISP,
};

/*
 * Define data structures.
 */

/* define control table entry */
struct emi_bwl_ctrl {
	unsigned int ref_cnt;
};

typedef struct {
	unsigned int dram_type;
	unsigned int ch_num;
	unsigned int rk_num;
	unsigned int rank_size[MAX_RK];
} emi_info_t;

/*
 * Define function prototype.
 */

extern int mtk_mem_bw_ctrl(int sce, int op);
extern unsigned int get_dram_type(void);
extern unsigned int get_ch_num(void);
extern unsigned int get_rk_num(void);
extern unsigned int get_rank_size(unsigned int rank_index); /* unit: all channels */
extern void __iomem *mt_emi_base_get(void); /* legacy API */
extern void __iomem *mt_cen_emi_base_get(void);
extern void __iomem *mt_chn_emi_base_get(int chn);
extern void __iomem *mt_emi_mpu_base_get(void);
extern void enable_drs(unsigned char enable);
extern int disable_drs(unsigned char *backup);
extern int DRS_enable(void);
extern int DRS_disable(void);
extern unsigned long long get_drs_all_self_cnt(unsigned int ch);
extern unsigned long long get_drs_rank1_self_cnt(unsigned int ch);
extern unsigned int mask_master_disable_drs(unsigned int master);
extern unsigned int unmask_master_disable_drs(unsigned int master);

#endif  /* !__MT_EMI_BWL_H__ */


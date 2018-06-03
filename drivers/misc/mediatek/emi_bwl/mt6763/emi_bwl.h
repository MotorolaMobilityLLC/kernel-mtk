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

#define EMI_CONM		(CEN_EMI_BASE + 0x0060)
#define EMI_ARBA		(CEN_EMI_BASE + 0x0100)
#define EMI_ARBB		(CEN_EMI_BASE + 0x0108)
#define EMI_ARBC		(CEN_EMI_BASE + 0x0110)
#define EMI_ARBD		(CEN_EMI_BASE + 0x0118)
#define EMI_ARBE		(CEN_EMI_BASE + 0x0120)
#define EMI_ARBF		(CEN_EMI_BASE + 0x0128)
#define EMI_ARBG		(CEN_EMI_BASE + 0x0130)
#define EMI_ARBH		(CEN_EMI_BASE + 0x0138)

/*
 * Define constants.
 */

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

/*
 * Define data structures.
 */

/* define control table entry */
struct emi_bwl_ctrl {
	unsigned int ref_cnt;
};

/*
 * Define function prototype.
 */

extern int mtk_mem_bw_ctrl(int sce, int op);
extern int get_ddr_type(void);
extern void __iomem *mt_cen_emi_base_get(void);
extern void __iomem *mt_chn_emi_base_get(int chn);
extern void __iomem *mt_emi_mpu_base_get(void);

#endif  /* !__MT_EMI_BWL_H__ */


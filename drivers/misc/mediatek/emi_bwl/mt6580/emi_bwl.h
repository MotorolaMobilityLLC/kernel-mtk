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

#define EMI_CONA     (0x0000)
#define EMI_CONB     (0x0008)
#define EMI_CONC     (0x0010)
#define EMI_COND     (0x0018)
#define EMI_CONE     (0x0020)
#define EMI_CONF     (0x0028)
#define EMI_CONG     (0x0030)
#define EMI_CONH     (0x0038)
#define EMI_TESTB    (0x0E8)
#define EMI_TESTD    (0x0F8)
#define EMI_ARBA     (0x0100)
#define EMI_ARBB     (0x0108)
#define EMI_ARBC     (0x0110)
#define EMI_ARBD     (0x0118)
#define EMI_ARBE     (0x0120)
#define EMI_ARBF     (0x0128)
#define EMI_ARBG     (0x0130)
#define EMI_ARBG_2ND (0x0134)
#define EMI_ARBH     (0x0138)
#define EMI_ARBI     (0x0140)
#define EMI_ARBI_2ND (0x0144)
#define EMI_ARBJ     (0x0148)
#define EMI_ARBJ_2ND (0x014C)
#define EMI_ARBK     (0x0150)
#define EMI_ARBK_2ND (0x0154)
#define EMI_SLCT     (0x0158)

#define DRAMC_CONF1   (0x004)
#define DRAMC_LPDDR2  (0x1e0)
#define DRAMC_PADCTL4 (0x0e4)
#define DRAMC_ACTIM1  (0x1e8)
#define DRAMC_DQSCAL0 (0x1c0)

#define DRAMC_READ(offset) (					\
		readl(IOMEM(DRAMC0_BASE + (offset)))|		\
		readl(IOMEM(DDRPHY_BASE + (offset)))|		\
		readl(IOMEM(DRAMC_NAO_BASE + (offset))))

#define DRAMC_WRITE(offset, data) do { \
		writel((unsigned int) (data), (DRAMC0_BASE + (offset))); \
		writel((unsigned int) (data), (DDRPHY_BASE + (offset))); \
		mt65xx_reg_sync_writel((unsigned int) (data),		 \
				       (DRAMC_NAO_BASE + (offset)));	 \
	} while (0)



/*
 * Define constants.
 */

/* define supported DRAM types */
enum {
	LPDDR2_1066 = 0,
	LPDDR3_1066,
	mDDR,
};

/* define concurrency scenario ID */
enum {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe, arbf, arbg) con_sce,
#include "con_sce_lpddr2_1066.h"
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

#endif  /* !__MT_EMI_BWL_H__ */


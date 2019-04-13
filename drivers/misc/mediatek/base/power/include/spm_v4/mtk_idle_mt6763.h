/*
 * Copyright (C) 2017 MediaTek Inc.
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

#ifndef __MTK_IDLE_MT6763_H__
#define __MTK_IDLE_MT6763_H__

#include <linux/io.h>


enum {
	UNIV_PLL = 0,
	MFG_PLL,
	MSDC_PLL,
	TVD_PLL,
	MM_PLL,
	NR_PLLS,
};

enum {
	CG_INFRA_0  = 0,
	CG_INFRA_1,
	CG_INFRA_2,
	CG_MMSYS0,
	CG_MMSYS1,
	CG_IMAGE,
	CG_MFG,
	CG_VCODEC,
	NR_GRPS,
};

#define NF_CG_STA_RECORD	(NR_GRPS + 2)

/* Only for code consistency */
#define NF_CLK_CFG            (1)

extern bool soidle_by_pass_pg;
extern bool dpidle_by_pass_pg;

extern void __iomem *infrasys_base;
extern void __iomem *mmsys_base;
extern void __iomem *imgsys_base;
extern void __iomem *mfgsys_base;
extern void __iomem *vcodecsys_base;
extern void __iomem *sleepsys_base;
extern void __iomem  *apmixed_base_in_idle;

#define INFRA_REG(ofs)		(infrasys_base + ofs)
#define MM_REG(ofs)		(mmsys_base + ofs)
#define IMGSYS_REG(ofs)		(imgsys_base + ofs)
#define MFGSYS_REG(ofs)		(mfgsys_base + ofs)
#define VENCSYS_REG(ofs)	(vencsys_base + ofs)
#define SPM_REG(ofs)		(sleepsys_base + ofs)
#define APMIXEDSYS(ofs)		(apmixed_base_in_idle + ofs)

#ifdef SPM_PWR_STATUS
#undef SPM_PWR_STATUS
#endif

#define	INFRA_SW_CG_0_STA	INFRA_REG(0x0094)
#define	INFRA_SW_CG_1_STA	INFRA_REG(0x0090)
#define	INFRA_SW_CG_2_STA	INFRA_REG(0x00AC)
#define DISP_CG_CON_0		MM_REG(0x100)
#define DISP_CG_CON_1		MM_REG(0x110)
#define IMG_CG_CON		IMGSYS_REG(0x0)
#define MFG_CG_CON		MFGSYS_REG(0x0)
#define VENCSYS_CG_CON		VENCSYS_REG(0x0)

#define SPM_PWR_STATUS		SPM_REG(0x0180)
#define SPM_PWR_STATUS_2ND	SPM_REG(0x0184)
#define SPM_ISP_PWR_CON		SPM_REG(0x0308)
#define SPM_MFG_PWR_CON		SPM_REG(0x0338)
#define SPM_VDE_PWR_CON		SPM_REG(0x0300)
#define SPM_VEN_PWR_CON		SPM_REG(0x0304)
#define SPM_DIS_PWR_CON		SPM_REG(0x030c)
#define SPM_AUDIO_PWR_CON	SPM_REG(0x0314)
#define SPM_MD1_PWR_CON		SPM_REG(0x0320)
#define SPM_MD2_PWR_CON		SPM_REG(0x0324)
#define SPM_C2K_PWR_CON		SPM_REG(0x0328)
#define SPM_CONN_PWR_CO		SPM_REG(0x032c)
#define SPM_MDSYS_INTF_INFRA_PWR_CON SPM_REG(0x0360)

/* FIXME: remove if no need to check audio clock mux */
#define CLK_CFG_0_BASE		TOPCKSYS_REG(0x100)
#define CLK_CFG_0_SET_BASE	TOPCKSYS_REG(0x104)
#define CLK_CFG_0_CLR_BASE	TOPCKSYS_REG(0x108)
#define CLK_CFG(n)		(CLK_CFG_0_BASE + n * 0x10)
#define CLK_CFG_SET(n)		(CLK_CFG_0_SET_BASE + n * 0x10)
#define CLK_CFG_CLR(n)		(CLK_CFG_0_CLR_BASE + n * 0x10)
#define CLK6_AUDINTBUS_MASK	0x700

#define ARMCA15PLL_BASE		APMIXEDSYS(0x200)
#define ARMCA15PLL_CON0		APMIXEDSYS(0x0200)
#define ARMCA7PLL_CON0		APMIXEDSYS(0x0210)
#define MAINPLL_CON0		APMIXEDSYS(0x0220)
#define UNIVPLL_CON0		APMIXEDSYS(0x0230)
#define MFGPLL_CON0		APMIXEDSYS(0x0240)
#define MSDCPLL_CON0		APMIXEDSYS(0x0250)
#define TVDPLL_CON0		APMIXEDSYS(0x0260)
#define MMPLL_CON0		APMIXEDSYS(0x0270)
#define APLL1_CON0		APMIXEDSYS(0x02a0)
#define APLL2_CON0		APMIXEDSYS(0x02b4)

#define DIS_PWR_STA_MASK        BIT(3)
#define MFG_PWR_STA_MASK        BIT(4)
#define ISP_PWR_STA_MASK        BIT(5)
#define VEN_PWR_STA_MASK        BIT(21)

#endif	/* __MTK_IDLE_MT6763_H__ */

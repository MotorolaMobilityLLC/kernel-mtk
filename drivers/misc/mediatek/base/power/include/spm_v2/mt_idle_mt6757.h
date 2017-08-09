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

#ifndef __MT_IDLE_MT6757__H__
#define __MT_IDLE_MT6757__H__

enum {
	UNIV_PLL = 0,
	MM_PLL,
	MSDC_PLL,
	VENC_PLL,
	NR_PLLS,
};

enum {
	CG_INFRA0  = 0,
	CG_INFRA1,
	CG_INFRA2,
	CG_DISP0,
	CG_DISP1,
	CG_IMAGE,
	CG_MFG,
	CG_VDEC,
	CG_VENC,
	CG_AUDIO,

	NR_GRPS
};

extern bool             soidle_by_pass_pg;
extern bool             dpidle_by_pass_pg;

extern void __iomem *infrasys_base;
extern void __iomem *mmsys_base;
extern void __iomem *sleepsys_base;
extern void __iomem *topcksys_base;
extern void __iomem *mfgsys_base;
extern void __iomem *imgsys_base;
extern void __iomem *vdecsys_base;
extern void __iomem *vencsys_base;
extern void __iomem *audiosys_base_in_idle;
extern void __iomem  *apmixed_base_in_idle;

#define INFRA_REG(ofs)      (infrasys_base + ofs)
#define MM_REG(ofs)         (mmsys_base + ofs)
#define SPM_REG(ofs)        (sleepsys_base + ofs)
#define TOPCKSYS_REG(ofs)	(topcksys_base + ofs)
#define MFGSYS_REG(ofs)     (mfgsys_base + ofs)
#define IMGSYS_REG(ofs)     (imgsys_base + ofs)
#define VDECSYS_REG(ofs)    (vdecsys_base + ofs)
#define VENCSYS_REG(ofs)    (vencsys_base + ofs)
#define AUDIOSYS_REG(ofs)   (audiosys_base_in_idle + ofs)
#define APMIXEDSYS(ofs)	    (apmixed_base_in_idle + ofs)

#ifdef SPM_PWR_STATUS
#undef SPM_PWR_STATUS
#endif

#ifdef SPM_PWR_STATUS_2ND
#undef SPM_PWR_STATUS_2ND
#endif

#define	INFRA_SW_CG_0_STA   INFRA_REG(0x0094)
#define	INFRA_SW_CG_1_STA   INFRA_REG(0x0090)
#define	INFRA_SW_CG_2_STA   INFRA_REG(0x00AC)
#define DISP_CG_CON0        MM_REG(0x100)
#define DISP_CG_CON1        MM_REG(0x110)
#define DISP_CG_DUMMY1      MM_REG(0x8A8)
#define DISP_CG_DUMMY2      MM_REG(0x8AC)

#define AUDIO_TOP_CON0      AUDIOSYS_REG(0x0)

#define SPM_PWR_STATUS      SPM_REG(0x0180)
#define SPM_PWR_STATUS_2ND  SPM_REG(0x0184)
#define SPM_ISP_PWR_CON     SPM_REG(0x0308)
#define SPM_MFG_PWR_CON     SPM_REG(0x0338)
#define SPM_VDE_PWR_CON     SPM_REG(0x0300)
#define SPM_VEN_PWR_CON     SPM_REG(0x0304)
#define SPM_DIS_PWR_CON     SPM_REG(0x030c)
#define SPM_AUDIO_PWR_CON   SPM_REG(0x0314)
#define SPM_MD1_PWR_CON   SPM_REG(0x0320)
#define SPM_MD2_PWR_CON   SPM_REG(0x0324)
#define SPM_C2K_PWR_CON   SPM_REG(0x0328)
#define SPM_CONN_PWR_CON   SPM_REG(0x032c)
#define SPM_MDSYS_INTF_INFRA_PWR_CON SPM_REG(0x0360)

#define	CLK_CFG_UPDATE          TOPCKSYS_REG(0x004)
#define CLK_CFG_4               TOPCKSYS_REG(0x080)
#define CLK_CFG_7               TOPCKSYS_REG(0x0B0)

#define ARMCA15PLL_BASE		APMIXEDSYS(0x200)
#define ARMCA15PLL_CON0		APMIXEDSYS(0x0200)
#define ARMCA7PLL_CON0		APMIXEDSYS(0x0210)
#define MAINPLL_CON0		APMIXEDSYS(0x0220)
#define UNIVPLL_CON0		APMIXEDSYS(0x0230)
#define MMPLL_CON0			APMIXEDSYS(0x0240)
#define MSDCPLL_CON0		APMIXEDSYS(0x0250)
#define VENCPLL_CON0		APMIXEDSYS(0x0260)
#define TVDPLL_CON0			APMIXEDSYS(0x0270)
#define APLL1_CON0			APMIXEDSYS(0x02a0)
#define APLL2_CON0			APMIXEDSYS(0x02b4)

#define MFG_CG_CON          MFGSYS_REG(0x0)
#define IMG_CG_CON          IMGSYS_REG(0x0)
#define VDEC_CG_CON_0       VDECSYS_REG(0x0)
#define VDEC_CG_CON_1       VDECSYS_REG(0x8)
#define VENCSYS_CG_CON      VENCSYS_REG(0x0)

#define DIS_PWR_STA_MASK        BIT(3)
#define MFG_PWR_STA_MASK        BIT(4)
#define ISP_PWR_STA_MASK        BIT(5)
#define VDE_PWR_STA_MASK        BIT(7)
#define VEN_PWR_STA_MASK        BIT(21)
#define MFG_2D_PWR_STA_MASK     BIT(22)
#define MFG_ASYNC_PWR_STA_MASK  BIT(23)
#define AUDIO_PWR_STA_MASK      BIT(24)


#define MUX_DISP_PWM             CLK_CFG_7
#define DISP_PWM_CLK_26M         0
#define DISP_PWM_UNIVPLL2_D4     1
#define DISP_PWM_LPOSC_D4        2
#define DISP_PWM_LPOSC_D8        3
#define DISP_PWM_LPOSC_MASK      (BIT(2))
#define AUXADC_CG_STA            BIT(10)

#endif /* __MT_IDLE_MT6757__H__ */


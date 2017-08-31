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

#ifndef __MTK_IDLE_INTERNAL_H__
#define __MTK_IDLE_INTERNAL_H__
#include <linux/io.h>

/*
 * Chip specific declaratinos
 */
#include <mtk_idle_internal.h>

enum {
	UNIV_PLL = 0,
	MM_PLL,
	MSDC_PLL,
	VENC_PLL,
	NR_PLLS,
};

enum {
	CG_INFRA_0  = 0,
	CG_INFRA_1,
	CG_INFRA_2,
	CG_PERI_0,
	CG_PERI_1,
	CG_PERI_2,
	CG_PERI_3,
	CG_PERI_4,
	CG_PERI_5,
	CG_AUDIO_0,
	CG_AUDIO_1,
	CG_DISP_0,
	CG_DISP_1,
	CG_DISP_2,
	CG_CAM,
	CG_IMAGE,
	CG_MFG,
	CG_VDEC_0,
	CG_VDEC_1,
	CG_VENC_0,
	CG_VENC_1,
	CG_MJC,
	CG_IPU,
	NR_GRPS,
};

#define NF_CG_STA_RECORD	(NR_GRPS + 2)

enum {
	PDN_MASK         = 0,
	PDN_VALUE,
	PUP_MASK,
	PUP_VALUE,
	NF_CLKMUX_MASK,
};

enum {
	/* CLK_CFG_0 */
	CLKMUX_MM             = 0,
	CLKMUX_DDRPHY         = 1,
	CLKMUX_MEM            = 2,
	CLKMUX_AXI            = 3,

	/* CLK_CFG_1 */
	CLKMUX_VDEC           = 4,
	CLKMUX_DISPPWM        = 5,
	CLKMUX_PWM            = 6,
	CLKMUX_SFLASH         = 7,

	/* CLK_CFG_2 */
	CLKMUX_I2C            = 8,
	CLKMUX_CAMTG          = 9,
	CLKMUX_MFG            = 10,
	CLKMUX_VENC           = 11,

	/* CLK_CFG_3 */
	CLKMUX_USB20          = 12,
	CLKMUX_AXI_PERI       = 13,
	CLKMUX_SPI            = 14,
	CLKMUX_UART           = 15,

	/* CLK_CFG_4 */
	CLKMUX_MSDC30_1       = 16,
	CLKMUX_MSDC50_0       = 17,
	CLKMUX_MSDC50_0_HCLK  = 18,
	CLKMUX_USB30_P0       = 19,

	/* CLK_CFG_5 */
	CLKMUX_SMI0_2X        = 20,
	CLKMUX_MSDC50_3_HCLK  = 21,
	CLKMUX_MSDC30_3       = 22,
	CLKMUX_I3C            = 23,

	/* CLK_CFG_6 */
	CLKMUX_SCP            = 24,
	CLKMUX_PMICSPI        = 25,
	CLKMUX_AUD_INTBUS     = 26,
	CLKMUX_AUDIO          = 27,

	/* CLK_CFG_7 */
	CLKMUX_DSP            = 28,
	CLKMUX_DPI0           = 29,
	CLKMUX_MJC            = 30,
	CLKMUX_ATB            = 31,

	/* CLK_CFG_8 */
	CLKMUX_AUD_ENGEN2     = 32,
	CLKMUX_AUD_ENGEN1     = 33,
	CLKMUX_AUD_2          = 34,
	CLKMUX_AUD_1          = 35,

	/* CLK_CFG_9 */
	CLKMUX_SENINF         = 36,
	CLKMUX_IPU_IF         = 37,
	CLKMUX_CAM            = 38,
	CLKMUX_DFP_MFG        = 39,

	/* CLK_CFG_10 */
	CLKMUX_UFO_DEC        = 40,
	CLKMUX_UFO_ENC        = 41,
	CLKMUX_IMG            = 42,
	CLKMUX_AXI_MFG_IN_AS  = 43,

	/* CLK_CFG_11 */
	CLKMUX_AES_FDE        = 44,
	CLKMUX_AES_UFSFDE     = 45,
	CLKMUX_EMI            = 46,
	CLKMUX_PCIE_MAC       = 47,

	/* CLK_CFG_12 */
	CLKMUX_SLOW_MFG       = 48,
	CLKMUX_ANCMD32        = 49,
	CLKMUX_SSPM           = 50,
	CLKMUX_AUDIO_H        = 51,

	/* CLK_CFG_13 */
	CLKMUX_SMI1_2X        = 52,
	CLKMUX_DXCC           = 53,
	CLKMUX_BSI_SPI        = 54,
	CLKMUX_UFS_CARD       = 55,

	/* CLK_CFG_14 */
	CLKMUX_RSV_0          = 56,
	CLKMUX_RSV_1          = 57,
	CLKMUX_RSV_2          = 58,
	CLKMUX_DFP            = 59,

	NF_CLKMUX,
};

#define NF_CLK_CFG            15     /* = NF_CLKMUX / 4 */

extern bool             soidle_by_pass_pg;
extern bool             mcsodi_by_pass_pg;
extern bool             dpidle_by_pass_pg;

extern void __iomem *infrasys_base;
extern void __iomem *perisys_base;
extern void __iomem *audiosys_base_in_idle;
extern void __iomem *mmsys_base;
extern void __iomem *camsys_base;
extern void __iomem *imgsys_base;
extern void __iomem *mfgsys_base;
extern void __iomem *vdecsys_base;
extern void __iomem *vencsys_global_base;
extern void __iomem *vencsys_base;
extern void __iomem *mjcsys_base;
extern void __iomem *ipusys_base;
extern void __iomem *topcksys_base;

extern void __iomem *sleepsys_base;
extern void __iomem  *apmixed_base_in_idle;

#define INFRA_REG(ofs)        (infrasys_base + ofs)
#define PERI_REG(ofs)         (perisys_base + ofs)
#define AUDIOSYS_REG(ofs)     (audiosys_base_in_idle + ofs)
#define MM_REG(ofs)           (mmsys_base + ofs)
#define CAMSYS_REG(ofs)       (camsys_base + ofs)
#define IMGSYS_REG(ofs)       (imgsys_base + ofs)
#define MFGSYS_REG(ofs)       (mfgsys_base + ofs)
#define VDECSYS_REG(ofs)      (vdecsys_base + ofs)
#define VENC_GLOBAL_REG(ofs)  (vencsys_global_base + ofs)
#define VENCSYS_REG(ofs)      (vencsys_base + ofs)
#define MJCSYS_REG(ofs)       (mjcsys_base + ofs)
#define IPUSYS_REG(ofs)       (ipusys_base + ofs)

#define SPM_REG(ofs)          (sleepsys_base + ofs)
#define TOPCKSYS_REG(ofs)     (topcksys_base + ofs)
#define APMIXEDSYS(ofs)	      (apmixed_base_in_idle + ofs)

#ifdef SPM_PWR_STATUS
#undef SPM_PWR_STATUS
#endif

#ifdef SPM_PWR_STATUS_2ND
#undef SPM_PWR_STATUS_2ND
#endif

#define	INFRA_SW_CG_0_STA     INFRA_REG(0x0094)
#define	INFRA_SW_CG_1_STA     INFRA_REG(0x0090)
#define	INFRA_SW_CG_2_STA     INFRA_REG(0x00AC)
#define PERI_SW_CG_0_STA      PERI_REG(0x0278)
#define PERI_SW_CG_1_STA      PERI_REG(0x0288)
#define PERI_SW_CG_2_STA      PERI_REG(0x0298)
#define PERI_SW_CG_3_STA      PERI_REG(0x02A8)
#define PERI_SW_CG_4_STA      PERI_REG(0x02B8)
#define PERI_SW_CG_5_STA      PERI_REG(0x02C8)
#define AUDIO_TOP_CON_0       AUDIOSYS_REG(0x0)
#define AUDIO_TOP_CON_1       AUDIOSYS_REG(0x4)
#define DISP_CG_CON_0         MM_REG(0x100)
#define DISP_CG_CON_1         MM_REG(0x110)
#define DISP_CG_CON_2         MM_REG(0x140)
#define CAMSYS_CG_CON         CAMSYS_REG(0x0)
#define IMG_CG_CON            IMGSYS_REG(0x0)
#define MFG_CG_CON            MFGSYS_REG(0x0)
#define VDEC_CKEN_SET         VDECSYS_REG(0x0)
#define VDEC_LARB1_CKEN_SET   VDECSYS_REG(0x8)
#define VENC_CE               VENC_GLOBAL_REG(0xEC)
#define VENCSYS_CG_CON        VENCSYS_REG(0x0)
#define MJC_SW_CG_CON         MJCSYS_REG(0x0)
#define IPU_CG_CON            IPUSYS_REG(0x0)

#define SPM_PWR_STATUS      SPM_REG(0x0190)
#define SPM_PWR_STATUS_2ND  SPM_REG(0x0194)

#define CLK_CFG_0_BASE        TOPCKSYS_REG(0x100)
#define CLK_CFG_0_SET_BASE    TOPCKSYS_REG(0x104)
#define CLK_CFG_0_CLR_BASE    TOPCKSYS_REG(0x108)
#define CLK_CFG(n)            (CLK_CFG_0_BASE + n * 0x10)
#define CLK_CFG_SET(n)        (CLK_CFG_0_SET_BASE + n * 0x10)
#define CLK_CFG_CLR(n)        (CLK_CFG_0_CLR_BASE + n * 0x10)
#define CLK6_AUDINTBUS_MASK   0x700

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

#define SC_MFG0_PWR_ACK         BIT(1)
#define SC_MFG1_PWR_ACK         BIT(2)
#define SC_MFG2_PWR_ACK         BIT(3)
#define SC_MFG3_PWR_ACK         BIT(4)
#define SC_INFRA_PWR_ACK        BIT(11)
#define SC_PERI_PWR_ACK         BIT(12)
#define SC_AUD_PWR_ACK          BIT(13)
#define SC_MJC_PWR_ACK          BIT(14)
#define SC_MM0_PWR_ACK          BIT(15)
#define SC_MM1_PWR_ACK          BIT(16)
#define SC_CAM_PWR_ACK          BIT(17)
#define SC_IPU_PWR_ACK          BIT(18)
#define SC_ISP_PWR_ACK          BIT(19)
#define SC_VEN_PWR_ACK          BIT(20)
#define SC_VDE_PWR_ACK          BIT(21)

/*
 * Common declarations
 */
enum mt_idle_mode {
	MT_DPIDLE = 0,
	MT_SOIDLE,
	MT_MCIDLE,
	MT_SLIDLE
};

/* IDLE_TYPE is used for idle_switch in mt_idle.c */
enum {
	IDLE_TYPE_DP = 0,
	IDLE_TYPE_SO3,
	IDLE_TYPE_SO,
	IDLE_TYPE_MC,
	IDLE_TYPE_MCSO,
	IDLE_TYPE_SL,
	IDLE_TYPE_RG,
	NR_TYPES,
};

/* CPUIDLE_STATE is used to represent CPUidle C States */
enum {
	CPUIDLE_STATE_RG = 0,
	CPUIDLE_STATE_SL,
	CPUIDLE_STATE_MCSO,
	CPUIDLE_STATE_MC,
	CPUIDLE_STATE_SO,
	CPUIDLE_STATE_DP,
	CPUIDLE_STATE_SO3,
	NR_CPUIDLE_STATE
};

enum {
	BY_CPU = 0,
	BY_CLK,
	BY_TMR,
	BY_OTH,
	BY_VTG,
	BY_FRM,
	BY_PLL,
	BY_PWM,
	BY_DCS,
	BY_UFS,
	BY_GPU,
	BY_SSPM_IPI,
	NR_REASONS,
};

#define INVALID_GRP_ID(grp)     (grp < 0 || grp >= NR_GRPS)
#define INVALID_IDLE_ID(id)     (id < 0 || id >= NR_TYPES)
#define INVALID_REASON_ID(id)   (id < 0 || id >= NR_REASONS)

#define idle_readl(addr)	    __raw_readl(addr)

extern unsigned int dpidle_blocking_stat[NR_GRPS][32];
extern int idle_switch[NR_TYPES];

extern unsigned int idle_condition_mask[NR_TYPES][NR_GRPS];

extern unsigned int soidle3_pll_condition_mask[NR_PLLS];

/*
 * Function Declarations
 */
const char *mtk_get_idle_name(int id);
const char *mtk_get_reason_name(int);
const char *mtk_get_cg_group_name(int id);
const char *mtk_get_pll_group_name(int id);

bool mtk_idle_check_cg(unsigned int block_mask[NR_TYPES][NF_CG_STA_RECORD]);
bool mtk_idle_check_secure_cg(unsigned int block_mask[NR_TYPES][NF_CG_STA_RECORD]);
bool mtk_idle_check_pll(unsigned int *condition_mask, unsigned int *block_mask);
bool mtk_idle_check_clkmux(int idle_type,
							unsigned int block_mask[NR_TYPES][NF_CLK_CFG]);

void __init iomap_init(void);

bool mtk_idle_disp_is_pwm_rosc(void);

#endif /* __MTK_IDLE_INTERNAL_H__ */


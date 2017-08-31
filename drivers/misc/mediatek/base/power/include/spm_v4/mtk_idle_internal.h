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

#ifndef __MTK_IDLE_INTERNAL_H__
#define __MTK_IDLE_INTERNAL_H__
#include <linux/io.h>

/*
 * Chip specific declaratinos
 */
#include "mtk_idle_internal.h"
#include "mtk_idle_mcdi.h"

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

#ifdef SPM_PWR_STATUS_2ND
#undef SPM_PWR_STATUS_2ND
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

/*
 * Common declarations
 */
enum mt_idle_mode {
	MT_DPIDLE = 0,
	MT_SOIDLE,
	MT_MCIDLE,
	MT_SLIDLE
};

/* CPUIDLE_STATE is used to represent CPUidle C States */
enum {
	CPUIDLE_STATE_RG = 0,
	CPUIDLE_STATE_SL,
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
	NR_REASONS,
};

#define INVALID_GRP_ID(grp)	(grp < 0 || grp >= NR_GRPS)
#define INVALID_IDLE_ID(id)	(id < 0 || id >= NR_TYPES)
#define INVALID_REASON_ID(id)	(id < 0 || id >= NR_REASONS)

#define idle_readl(addr)	__raw_readl(addr)

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

void __init iomap_init(void);

bool mtk_idle_disp_is_pwm_rosc(void);

unsigned int soidle_pre_handler(void);
void soidle_post_handler(void);


enum {
	PIDX_SELECT,
	PIDX_SELECT_TO_ENTER,
	PIDX_ENTER_TOTAL,
	PIDX_LEAVE_TOTAL,
	PIDX_IDLE_NOTIFY_ENTER,
	PIDX_PRE_HANDLER,
	PIDX_SSPM_BEFORE_WFI,
	PIDX_PRE_IRQ_PROCESS,
	PIDX_PCM_SETUP_BEFORE_WFI,
	PIDX_SSPM_BEFORE_WFI_ASYNC_WAIT,
	PIDX_SSPM_AFTER_WFI,
	PIDX_PCM_SETUP_AFTER_WFI,
	PIDX_POST_IRQ_PROCESS,
	PIDX_POST_HANDLER,
	PIDX_SSPM_AFTER_WFI_ASYNC_WAIT,
	PIDX_IDLE_NOTIFY_LEAVE,
	NR_PIDX
};

#if defined(SPM_SODI_PROFILE_TIME) || defined(SPM_SODI3_PROFILE_TIME)
void idle_latency_profile(unsigned int idle_type, int idx);
void idle_latency_dump(unsigned int idle_type);
#endif /* defined(SPM_SODI_PROFILE_TIME) || defined(SPM_SODI3_PROFILE_TIME) */

#ifdef SPM_SODI_PROFILE_TIME
#define profile_so_start(idx)   idle_latency_profile(IDLE_TYPE_SO, 2*idx)
#define profile_so_end(idx)     idle_latency_profile(IDLE_TYPE_SO, 2*idx+1)
#define profile_so_dump()       idle_latency_dump(IDLE_TYPE_SO)
#else
#define profile_so_start(idx)
#define profile_so_end(idx)
#define profile_so_dump()
#endif /* SPM_SODI_PROFILE_TIME */

#ifdef SPM_SODI3_PROFILE_TIME
#define profile_so3_start(idx)  idle_latency_profile(IDLE_TYPE_SO3, 2*idx)
#define profile_so3_end(idx)    idle_latency_profile(IDLE_TYPE_SO3, 2*idx+1)
#define profile_so3_dump()      idle_latency_dump(IDLE_TYPE_SO3)
#else
#define profile_so3_start(idx)
#define profile_so3_end(idx)
#define profile_so3_dump()
#endif /* SPM_SODI3_PROFILE_TIME */

#endif /* __MTK_IDLE_INTERNAL_H__ */


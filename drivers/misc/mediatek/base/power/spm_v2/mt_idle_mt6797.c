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

#include <linux/of.h>
#include <linux/of_address.h>

#include <mach/mt_spm_mtcmos_internal.h>

#include <mt_spm_idle.h>
#include "mt_idle_internal.h"

#define IDLE_TAG     "Power/swap "
#define idle_err(fmt, args...)		pr_err(IDLE_TAG fmt, ##args)
#define idle_dbg(fmt, args...)		pr_debug(IDLE_TAG fmt, ##args)

enum subsys_id {
	SYS_DIS,
	SYS_ISP,
	SYS_VDE,
	SYS_MFG,
	SYS_MJC,
	SYS_VEN,
	SYS_AUDIO,
	NR_SYSS__,
};

/*
 * Variable Declarations
 */
void __iomem *infrasys_base;
void __iomem *mmsys_base;
void __iomem *sleepsys_base;
void __iomem *topcksys_base;
void __iomem *mfgsys_base;
void __iomem *imgsys_base;
void __iomem *vdecsys_base;
void __iomem *vencsys_base;
void __iomem *audiosys_base_in_idle;

void __iomem  *apmixed_base_in_idle;

/* Idle handler on/off */
int idle_switch[NR_TYPES] = {
	1,	/* dpidle switch */
	1,	/* soidle3 switch */
	1,	/* soidle switch */
#ifdef CONFIG_CPU_ISOLATION
	1,	/* mcidle switch */
#else
	0,	/* mcidle switch */
#endif
	0,	/* slidle switch */
	1,	/* rgidle switch */
};

unsigned int dpidle_blocking_stat[NR_GRPS][32];

unsigned int dpidle_condition_mask[NR_GRPS] = {
	0x00460802, /* INFRA0: */
	0x03AFF900, /* INFRA1: */
	0x21FE36FD, /* INFRA2: */
	0xFFFFFFFF, /* DISP0:  */
	0x000003FF, /* DISP1:  */
	0x00000312, /* IMAGE, use SPM MTCMOS off as condition: */
	0x00000012, /* MFG,   use SPM MTCMOS off as condition: */
	0x00000000, /* AUDIO */
	0x00000112, /* VDEC,  use SPM MTCMOS off as condition: */
	0x00000F12, /* VENC,  use SPM MTCMOS off as condition: */
	0x00000112, /* MJC,   use SPM MTCMOS off as condition: */
};

unsigned int soidle3_pll_condition_mask[NR_PLLS] = {
	1, /* UNIVPLL */
	0, /* MFGPLL */
	1, /* MSDCPLL */
	0, /* IMGPLL */
	0, /* TVDPLL*/
	0, /* MPLL*/
	0, /* CODECPLL*/
	0, /* MDPLL*/
	0, /* VENCPLL*/
};


unsigned int soidle3_condition_mask[NR_GRPS] = {
	0x02440802, /* INFRA0: separate AUXADC CG check */
	0x03AFF900, /* INFRA1: */
	0x2DFE36FD, /* INFRA2: */
	0x00507FF8, /* DISP0:  */
	0x000002F0, /* DISP1:  */
	0x00000312, /* IMAGE, use SPM MTCMOS off as condition: */
	0x00000012, /* MFG,   use SPM MTCMOS off as condition: */
	0x00000000, /* AUDIO */
	0x00000112, /* VDEC,  use SPM MTCMOS off as condition: */
	0x00000F12, /* VENC,  use SPM MTCMOS off as condition: */
	0x00000112, /* MJC,   use SPM MTCMOS off as condition: */
};

unsigned int soidle_condition_mask[NR_GRPS] = {
	0x00440802, /* INFRA0: */
	0x03AFF900, /* INFRA1: */
	0x21FE36FD, /* INFRA2: */
	0x00507FF8, /* DISP0:  */
	0x000002F0, /* DISP1:  */
	0x00000312, /* IMAGE, use SPM MTCMOS off as condition: */
	0x00000012, /* MFG,   use SPM MTCMOS off as condition: */
	0x00000000, /* AUDIO */
	0x00000112, /* VDEC,  use SPM MTCMOS off as condition: */
	0x00000F12, /* VENC,  use SPM MTCMOS off as condition: */
	0x00000112, /* MJC,   use SPM MTCMOS off as condition: */
};

unsigned int slidle_condition_mask[NR_GRPS] = {
	0x00000000, /* INFRA0: */
	0x00000000, /* INFRA1: */
	0x00000000, /* INFRA2: */
	0x00000000, /* DISP0:  */
	0x00000000, /* DISP1:  */
	0x00000000, /* IMAGE, use SPM MTCMOS off as condition: */
	0x00000000, /* MFG,   use SPM MTCMOS off as condition: */
	0x00000000, /* AUDIO */
	0x00000000, /* VDEC,  use SPM MTCMOS off as condition: */
	0x00000000, /* VENC,  use SPM MTCMOS off as condition: */
	0x00000000, /* MJC,   use SPM MTCMOS off as condition: */
};

const char *idle_name[NR_TYPES] = {
	"dpidle",
	"soidle3",
	"soidle",
	"mcidle",
	"slidle",
	"rgidle",
};

const char *reason_name[NR_REASONS] = {
	"by_cpu",
	"by_clk",
	"by_tmr",
	"by_oth",
	"by_vtg",
	"by_frm",
	"by_pll",
	"by_pwm",
#ifdef CONFIG_CPU_ISOLATION
	"by_iso",
#endif
	"by_dvfsp",
};

static char cg_group_name[NR_GRPS][10] = {
	"INFRA0",
	"INFRA1",
	"INFRA2",
	"DISP0",
	"DISP1",
	"IMAGE",
	"MFG",
	"AUDIO",
	"VDEC",
	"VENC",
	"MJC",
};

/*
 * Weak functions
 */
void __attribute__((weak))  msdc_clk_status(int *status)
{
	*status = 0;
}

/*
 * Function Definitions
 */
static int sys_is_on(enum subsys_id id)
{
	u32 pwr_sta_mask[] = {
		DIS_PWR_STA_MASK,
		ISP_PWR_STA_MASK,
		VDE_PWR_STA_MASK,
		MFG_PWR_STA_MASK,
		MJC_PWR_STA_MASK,
		VEN_PWR_STA_MASK,
		AUDIO_PWR_STA_MASK,
	};

	u32 mask = pwr_sta_mask[id];
	u32 sta = idle_readl(SPM_PWR_STATUS);
	u32 sta_s = idle_readl(SPM_PWR_STATUS_2ND);

	if (id >= NR_SYSS__)
		BUG();

	return (sta & mask) && (sta_s & mask);
}

#define	DISP0_CG_MASK	0x00004007		/* bit[14], bit[2:0] */
#define	DISP0_DCM_MASK	0xFFFFBFF8
#define	DISP1_CG_MASK	0xFFFFFEDF
#define	DISP1_DCM_MASK	0x00000120		/* bit[8], bit[5] */

static void get_all_clock_state(u32 clks[NR_GRPS])
{
	int i;

	for (i = 0; i < NR_GRPS; i++)
		clks[i] = 0;

	clks[CG_INFRA0] = ~idle_readl(INFRA_SW_CG_0_STA); /* INFRA0 */
	clks[CG_INFRA1] = ~idle_readl(INFRA_SW_CG_1_STA); /* INFRA1 */
	clks[CG_INFRA2] = ~idle_readl(INFRA_SW_CG_2_STA); /* INFRA2 */

	if (sys_is_on(SYS_DIS)) {
		clks[CG_DISP0] = (~idle_readl(DISP_CG_CON0) & DISP0_CG_MASK) |
							(idle_readl(DISP_CG_DUMMY1) & DISP0_DCM_MASK);
		clks[CG_DISP1] = (~idle_readl(DISP_CG_CON1) & DISP1_CG_MASK) |
							(idle_readl(DISP_CG_DUMMY2) & DISP1_DCM_MASK);
	}

	if (sys_is_on(SYS_ISP))
		clks[CG_IMAGE] = ~idle_readl(SPM_ISP_PWR_CON); /* IMAGE */

	if (sys_is_on(SYS_MFG))
		clks[CG_MFG] = ~idle_readl(SPM_MFG_PWR_CON); /* MFG */

	if (sys_is_on(SYS_VDE))
		clks[CG_VDEC] = ~idle_readl(SPM_VDE_PWR_CON); /* VDEC */

	if (sys_is_on(SYS_VEN))
		clks[CG_VENC] = ~idle_readl(SPM_VEN_PWR_CON); /* VENC */

	if (sys_is_on(SYS_MJC))
		clks[CG_MJC] = ~idle_readl(SPM_MJC_PWR_CON); /* VMJC */

	if (sys_is_on(SYS_AUDIO) && (clks[CG_INFRA2] & 0x04000000))
		clks[CG_AUDIO] = ~idle_readl(AUDIO_TOP_CON0); /* AUDIO */
}

bool cg_check_idle_can_enter(
	unsigned int *condition_mask, unsigned int *block_mask, enum mt_idle_mode mode)
{
	int i;
	unsigned int sd_mask = 0;
	u32 clks[NR_GRPS];
	u32 r = 0;
	unsigned int sta;
	bool ret = true;
	int k;

	/* SD status */
	msdc_clk_status(&sd_mask);
	if (sd_mask) {
		block_mask[CG_INFRA0] |= sd_mask;
		ret = false;
	}

	/* CG status */
	get_all_clock_state(clks);

	for (i = 0; i < NR_GRPS; i++) {
		block_mask[i] = condition_mask[i] & clks[i];
		r |= block_mask[i];
	}

	if (!(r == 0))
		ret = false;

	if (mode == MT_DPIDLE) {
		for (i = 0; i < NR_GRPS; i++) {
			for (k = 0; k < 32; k++) {
				if (block_mask[i] & (1 << k))
					dpidle_blocking_stat[i][k] += 1;
			}
		}
	}

	/* MTCMOS status */
	sta = idle_readl(SPM_PWR_STATUS);
	if (mode == MT_DPIDLE) {
		if (!dpidle_by_pass_pg) {
			if (sta & (MFG_PWR_STA_MASK |
						ISP_PWR_STA_MASK |
						VDE_PWR_STA_MASK |
						VEN_PWR_STA_MASK |
						MJC_PWR_STA_MASK |
						DIS_PWR_STA_MASK))
				return false;
		}
	} else if (mode == MT_SOIDLE) {
		if (!soidle_by_pass_pg) {
			if (sta & (MFG_PWR_STA_MASK |
						ISP_PWR_STA_MASK |
						VDE_PWR_STA_MASK |
						VEN_PWR_STA_MASK |
						MJC_PWR_STA_MASK))
				return false;
			if (spm_get_cmd_mode() && (sta & DIS_PWR_STA_MASK))
				return false;
		}
	}

	return ret;
}

bool cg_i2c_appm_check_idle_can_enter(unsigned int *block_mask)
{
	u32 clk_stat = ~idle_readl(INFRA_SW_CG_2_STA); /* INFRA1 */

	if ((clk_stat & 0x00000002) == 0x00000002) {
		block_mask[CG_INFRA2] |= 0x00000002;
		return false;
	}

	return true;
}

struct{
	const unsigned int offset;
	const char name[10];
} pll_info[NR_PLLS] = {
	{0x230, "UNIVPLL"},
	{0x240, "MFGPLL"},
	{0x250, "MSDCPLL"},
	{0x260, "IMGPLL"},
	{0x270, "TVDPLL"},
	{0x280, "MPLL"},
	{0x290, "CODECPLL"},
	{0x2c8, "MDPLL"},
	{0x2e4, "VENCPLL"},
};

const char *pll_grp_get_name(int id)
{
	return pll_info[id].name;
}

int is_pll_on(int id)
{
	return idle_readl(APMIXEDSYS(pll_info[id].offset)) & 0x1;
}

bool pll_check_idle_can_enter(unsigned int *condition_mask, unsigned int *block_mask)
{
	int i, j;

	memset(block_mask, 0, NR_PLLS * sizeof(unsigned int));

	for (i = 0; i < NR_PLLS; i++) {
		if (condition_mask[i] && is_pll_on(i)) {
			for (j = i; j < NR_PLLS; j++)
				block_mask[j] = is_pll_on(j) & condition_mask[j];

			return false;
		}
	}

	return true;
}

static int __init get_base_from_node(
	const char *cmp, void __iomem **pbase, int idx)
{
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, cmp);

	if (!node) {
		idle_err("node '%s' not found!\n", cmp);
		/* TODO: BUG() */
	}

	*pbase = of_iomap(node, idx);
	if (!(*pbase)) {
		idle_err("node '%s' cannot iomap!\n", cmp);
		/* TODO: BUG() */
	}

	return 0;
}

void __init iomap_init(void)
{
	get_base_from_node("mediatek,infracfg_ao", &infrasys_base, 0);
	get_base_from_node("mediatek,mmsys_config", &mmsys_base, 0);
	get_base_from_node("mediatek,sleep", &sleepsys_base, 0);
	get_base_from_node("mediatek,topckgen", &topcksys_base, 0);
	get_base_from_node("mediatek,apmixed", &apmixed_base_in_idle, 0);
	get_base_from_node("mediatek,g3d_config", &mfgsys_base, 0);
	get_base_from_node("mediatek,imgsys_config", &imgsys_base, 0);
	get_base_from_node("mediatek,mt6797-vdec_gcon", &vdecsys_base, 0);
	get_base_from_node("mediatek,mt6797-venc_gcon", &vencsys_base, 0);
	get_base_from_node("mediatek,audio", &audiosys_base_in_idle, 0);
}

const char *cg_grp_get_name(int id)
{
	BUG_ON(INVALID_GRP_ID(id));
	return cg_group_name[id];
}

bool is_disp_pwm_rosc(void)
{
	return (idle_readl(DISP_PWM_MUX) & PWM_LPOSC_MASK) != 0;
}

bool is_auxadc_released(void)
{
#if 0
	if (~idle_readl(INFRA_SW_CG_0_STA) & AUXADC_CG_STA) {
		idle_dbg("AUXADC CG does not be released\n");
		return false;
	}
#endif
	return true;
}

bool vcore_dvfs_is_progressing(void)
{
	return vcorefs_screen_on_lock_sodi();
}

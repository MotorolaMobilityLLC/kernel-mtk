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

/*#include <mach/mt_spm_mtcmos_internal.h>*/

#include <ddp_pwm.h>
#include <mtk_dramc.h>
#include <mtk_idle_internal.h>
#include <mtk_spm_idle.h>

#define IDLE_TAG "Power/swap "
#define idle_err(fmt, args...) pr_debug(IDLE_TAG fmt, ##args)
#define idle_dbg(fmt, args...) pr_debug(IDLE_TAG fmt, ##args)

enum subsys_id {
	SYS_DIS,
	SYS_MFG,
	SYS_ISP,
	SYS_VDE,
	SYS_VEN,
	SYS_MFG_2D,
	SYS_MFG_ASYNC,
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
void __iomem *apmixed_base_in_idle;

/* Idle handler on/off */
int idle_switch[NR_TYPES] = {
	1, /* dpidle switch */
	1, /* soidle3 switch */
	1, /* soidle switch */
#ifdef CONFIG_CPU_ISOLATION
	0, /* mcidle switch */
#else
	0, /* mcidle switch */
#endif
	0, /* slidle switch */
	1, /* rgidle switch */
};

unsigned int dpidle_blocking_stat[NR_GRPS][32];

unsigned int dpidle_condition_mask[NR_GRPS] = {
	0x00640802, /* INFRA0: */
	0x03AFB900, /* INFRA1: separate I2C-appm CG check */
	0x060006C5, /* INFRA2: USB blocking idle via spm_resource API */
	0xFFFFFFFF, /* DISP0:  */
	0x0000CFFF, /* DISP1:  */
	0x00000312, /* IMAGE, use SPM MTCMOS off as condition: */
	0x00000112, /* MFG,   use SPM MTCMOS off as condition: */
	0x00000112, /* VDEC,  use SPM MTCMOS off as condition: */
	0x00000F12, /* VENC,  use SPM MTCMOS off as condition: */
	0x00000000, /* AUDIO */
};

unsigned int soidle3_pll_condition_mask[NR_PLLS] = {
	0, /* UNIVPLL */
	0, /* MMPLL */
	1, /* MSDCPLL */
	0, /* VENCPLL */
};

unsigned int soidle3_condition_mask[NR_GRPS] = {
	0x02640802, /* INFRA0: separate AUXADC CG check */
	0x03AFB900, /* INFRA1: separate I2C-appm CG check */
	0x060006D1, /* INFRA2: USB blocking idle via spm_resource API */
	0xAAB06FF8, /* DISP0:  */
	0x00008C15, /* DISP1:  */
	0x00000312, /* IMAGE, use SPM MTCMOS off as condition: */
	0x00000112, /* MFG,   use SPM MTCMOS off as condition: */
	0x00000112, /* VDEC,  use SPM MTCMOS off as condition: */
	0x00000F12, /* VENC,  use SPM MTCMOS off as condition: */
	0x00000000, /* AUDIO */
};

unsigned int soidle_condition_mask[NR_GRPS] = {
	0x00640802, /* INFRA0: */
	0x03AFB900, /* INFRA1: separate I2C-appm CG check */
	0x060006C1, /* INFRA2: USB blocking idle via spm_resource API */
	0xAAB06FF8, /* DISP0:  */
	0x00008C15, /* DISP1:  */
	0x00000312, /* IMAGE, use SPM MTCMOS off as condition: */
	0x00000112, /* MFG,   use SPM MTCMOS off as condition: */
	0x00000112, /* VDEC,  use SPM MTCMOS off as condition: */
	0x00000F12, /* VENC,  use SPM MTCMOS off as condition: */
	0x00000000, /* AUDIO */
};

unsigned int slidle_condition_mask[NR_GRPS] = {
	0x00000000, /* INFRA0: */
	0x00000000, /* INFRA1: */
	0x00000000, /* INFRA2: */
	0x00000000, /* DISP0:  */
	0x00000000, /* DISP1:  */
	0x00000000, /* IMAGE, use SPM MTCMOS off as condition: */
	0x00000000, /* MFG,   use SPM MTCMOS off as condition: */
	0x00000000, /* VDEC,  use SPM MTCMOS off as condition: */
	0x00000000, /* VENC,  use SPM MTCMOS off as condition: */
	0x00000000, /* AUDIO */
};

const char *idle_name[NR_TYPES] = {
	"dpidle", "soidle3", "soidle", "mcidle", "slidle", "rgidle",
};

const char *reason_name[NR_REASONS] = {
	"by_cpu",   "by_clk", "by_tmr", "by_oth",
	"by_vtg",   "by_frm", "by_pll", "by_pwm",
#ifdef CONFIG_CPU_ISOLATION
	"by_iso",
#endif
	"by_dvfsp",
};

static char cg_group_name[NR_GRPS][10] = {
	"INFRA0", "INFRA1", "INFRA2", "DISP0", "DISP1",
	"IMAGE",  "MFG",    "VDEC",   "VENC",  "AUDIO",
};

struct pll_info {
	const unsigned int reg;
	const char name[10];
};

static struct pll_info pll_grp[NR_PLLS] = {
	{0x230, "UNIVPLL"},
	{0x240, "MMPLL"},
	{0x250, "MSDCPLL"},
	{0x260, "VENCPLL"},
};

/*
 * Weak functions
 */
void __attribute__((weak)) msdc_clk_status(int *status) { *status = 0; }

bool __attribute__((weak)) disp_pwm_is_osc(void) { return false; }

/*
 * Function Definitions
 */
static int sys_is_on(enum subsys_id id)
{
	u32 pwr_sta_mask[] = {
	    DIS_PWR_STA_MASK,       MFG_PWR_STA_MASK,   ISP_PWR_STA_MASK,
	    VDE_PWR_STA_MASK,       VEN_PWR_STA_MASK,   MFG_2D_PWR_STA_MASK,
	    MFG_ASYNC_PWR_STA_MASK, AUDIO_PWR_STA_MASK,
	};

	u32 mask = pwr_sta_mask[id];
	u32 sta = idle_readl(SPM_PWR_STATUS);
	u32 sta_s = idle_readl(SPM_PWR_STATUS_2ND);

	/*
	 *	if (id >= NR_SYSS__)
	 *		BUG();
	 */
	return (sta & mask) && (sta_s & mask);
}

#define DISP0_CG_MASK 0x00004007
#define DISP0_DCM_MASK 0xFFFFBFF8
#define DISP1_CG_MASK 0x0003BFC0
#define DISP1_DCM_MASK 0x0000403F

static void get_all_clock_state(u32 clks[NR_GRPS])
{
	int i;

	for (i = 0; i < NR_GRPS; i++)
		clks[i] = 0;

	clks[CG_INFRA0] = ~idle_readl(INFRA_SW_CG_0_STA); /* INFRA0 */
	clks[CG_INFRA1] = ~idle_readl(INFRA_SW_CG_1_STA); /* INFRA1 */
	clks[CG_INFRA2] = ~idle_readl(INFRA_SW_CG_2_STA); /* INFRA2 */

	if (sys_is_on(SYS_DIS) &&
	    (idle_readl(APMIXEDSYS(pll_grp[VENC_PLL].reg)) & 0x1)) {
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

	if (sys_is_on(SYS_AUDIO))
		clks[CG_AUDIO] = ~idle_readl(AUDIO_TOP_CON0); /* AUDIO */
}

bool cg_check_idle_can_enter(unsigned int *condition_mask,
			     unsigned int *block_mask, enum mtk_idle_mode mode)
{
	int i;
	unsigned int sd_mask = 0;
	u32 clks[NR_GRPS];
	u32 r = 0;
	unsigned int sta;
	bool ret = true;
	int k;

	/* CG status */
	get_all_clock_state(clks);

	for (i = 0; i < NR_GRPS; i++) {
		block_mask[i] = condition_mask[i] & clks[i];
		r |= block_mask[i];
	}

	if (!(r == 0))
		ret = false;

	/* SD status */
	msdc_clk_status(&sd_mask);
	if (sd_mask) {
		block_mask[CG_INFRA0] |= sd_mask;
		ret = false;
	}

	if (mode == MTK_DPIDLE) {
		for (i = 0; i < NR_GRPS; i++) {
			for (k = 0; k < 32; k++) {
				if (block_mask[i] & (1 << k))
					dpidle_blocking_stat[i][k] += 1;
			}
		}
	}

	/* MTCMOS status */
	sta = idle_readl(SPM_PWR_STATUS);
	if (mode == MTK_DPIDLE) {
		if (!dpidle_by_pass_pg) {
			if (sta & (MFG_PWR_STA_MASK | ISP_PWR_STA_MASK |
				   VDE_PWR_STA_MASK | VEN_PWR_STA_MASK |
				   DIS_PWR_STA_MASK))
				return false;
		}
	} else if (mode == MTK_SOIDLE) {
		if (!soidle_by_pass_pg) {
			if (sta & (MFG_PWR_STA_MASK | ISP_PWR_STA_MASK |
				   VDE_PWR_STA_MASK | VEN_PWR_STA_MASK))
				return false;
		}
	}

	return ret;
}

bool cg_i2c_appm_check_idle_can_enter(unsigned int *block_mask)
{
	u32 clk_stat = ~idle_readl(INFRA_SW_CG_1_STA); /* INFRA1 */

	if ((clk_stat & 0x00004000) == 0x00004000) {
		block_mask[CG_INFRA1] |= 0x00004000;
		return false;
	}

	return true;
}

const char *pll_grp_get_name(int id) { return pll_grp[id].name; }

int is_pll_on(int id) { return idle_readl(APMIXEDSYS(pll_grp[id].reg)) & 0x1; }

bool pll_check_idle_can_enter(unsigned int *condition_mask,
			      unsigned int *block_mask)
{
	int i, j;
	unsigned int pll_mask;

	memset(block_mask, 0, NR_PLLS * sizeof(unsigned int));

	for (i = 0; i < NR_PLLS; i++) {
		pll_mask = is_pll_on(i) & condition_mask[i];
		if (pll_mask) {
			for (j = 0; j < NR_PLLS; j++)
				block_mask[j] =
				    is_pll_on(j) & condition_mask[j];

			return false;
		}
	}

	return true;
}

static int __init get_base_from_node(const char *cmp, void __iomem **pbase,
				     int idx)
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
	get_base_from_node("mediatek,vdec_gcon", &vdecsys_base, 0);
	get_base_from_node("mediatek,venc_gcon", &vencsys_base, 0);
	get_base_from_node("mediatek,audio", &audiosys_base_in_idle, 0);
}

const char *cg_grp_get_name(int id)
{
	/*BUG_ON(INVALID_GRP_ID(id));*/
	return cg_group_name[id];
}

bool is_disp_pwm_rosc(void) { return disp_pwm_is_osc(); }

bool is_auxadc_released(void)
{
#if 0
	if ((~idle_readl(INFRA_SW_CG_0_STA) & AUXADC_CG_STA) != 0) {
		idle_err("AUXADC CG does not be released\n");
		return false;
	}
#endif
	return true;
}

bool univpll_is_off(void)
{
	return !(idle_readl(APMIXEDSYS(pll_grp[UNIV_PLL].reg)) & 0x1);
}

bool vcore_dvfs_is_progressing(void) { return false; }

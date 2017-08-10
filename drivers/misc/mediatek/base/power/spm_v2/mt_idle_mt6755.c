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

#include "mt_idle_internal.h"

#define IDLE_TAG     "Power/swap"
#define idle_err(fmt, args...)		pr_err(IDLE_TAG fmt, ##args)

enum subsys_id {
	SYS_VDE,
	SYS_MFG,
	SYS_VEN,
	SYS_ISP,
	SYS_DIS,
	SYS_AUDIO,
	SYS_MFG_2D,
	SYS_MFG_ASYNC,
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
#ifdef CONFIG_MTK_PMIC_CHIP_MT6353
	1,	/* soidle3 switch */
#else
	0,	/* soidle3 switch */
#endif
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
	0x00640802, /* INFRA0: */
	0x03AFB900, /* INFRA1: separate I2C-appm CG check */
	0x000000C7, /* INFRA2: */
	0x03FFFFFF, /* DISP0:  */
	0x00000312, /* IMAGE, use SPM MTCMOS off as condition: */
	0x00000112, /* MFG,   use SPM MTCMOS off as condition: */
	0x00000000, /* AUDIO */
	0x00000112, /* VDEC,  use SPM MTCMOS off as condition: */
	0x00000F12, /* VENC,  use SPM MTCMOS off as condition: */
};

unsigned int soidle3_pll_condition_mask[NR_PLLS] = {
	1, /* UNIVPLL */
	0, /* MMPLL */
	1, /* MSDCPLL */
	0, /* VENCPLL */
};

unsigned int soidle3_condition_mask[NR_GRPS] = {
	0x026C0802, /* INFRA0: separate AUXADC CG check */
	0x03AFB900, /* INFRA1: separate I2C-appm CG check */
	0x000000D3, /* INFRA2: */
	0x005023FC, /* DISP0:  */
	0x00000312, /* IMAGE, use SPM MTCMOS off as condition: */
	0x00000112, /* MFG,   use SPM MTCMOS off as condition: */
	0x00000000, /* AUDIO */
	0x00000112, /* VDEC,  use SPM MTCMOS off as condition: */
	0x00000F12, /* VENC,  use SPM MTCMOS off as condition: */
};

unsigned int soidle_condition_mask[NR_GRPS] = {
	0x00640802, /* INFRA0: */
	0x03AFB900, /* INFRA1: separate I2C-appm CG check */
	0x000000C3, /* INFRA2: */
	0x005023FC, /* DISP0:  */
	0x00000312, /* IMAGE, use SPM MTCMOS off as condition: */
	0x00000112, /* MFG,   use SPM MTCMOS off as condition: */
	0x00000000, /* AUDIO */
	0x00000112, /* VDEC,  use SPM MTCMOS off as condition: */
	0x00000F12, /* VENC,  use SPM MTCMOS off as condition: */
};

unsigned int slidle_condition_mask[NR_GRPS] = {
	0x00000000, /* INFRA0: */
	0x00000000, /* INFRA1: */
	0x00000000, /* INFRA2: */
	0x00000000, /* DISP0:  */
	0x00000000, /* IMAGE, use SPM MTCMOS off as condition: */
	0x00000000, /* MFG,   use SPM MTCMOS off as condition: */
	0x00000000, /* AUDIO */
	0x00000000, /* VDEC,  use SPM MTCMOS off as condition: */
	0x00000000, /* VENC,  use SPM MTCMOS off as condition: */
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
	"by_conn",
};

static char cg_group_name[NR_GRPS][10] = {
	"INFRA0",
	"INFRA1",
	"INFRA2",
	"DISP0",
	"IMAGE",
	"MFG",
	"AUDIO",
	"VDEC",
	"VENC",
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
		VDE_PWR_STA_MASK,
		MFG_PWR_STA_MASK,
		VEN_PWR_STA_MASK,
		ISP_PWR_STA_MASK,
		DIS_PWR_STA_MASK,
		AUDIO_PWR_STA_MASK,
		MFG_2D_PWR_STA_MASK,
		MFG_ASYNC_PWR_STA_MASK,
	};

	u32 mask = pwr_sta_mask[id];
	u32 sta = idle_readl(SPM_PWR_STATUS);
	u32 sta_s = idle_readl(SPM_PWR_STATUS_2ND);

	if (id >= NR_SYSS__)
		BUG();

	return (sta & mask) && (sta_s & mask);
}

static void get_all_clock_state(u32 clks[NR_GRPS])
{
	int i;

	for (i = 0; i < NR_GRPS; i++)
		clks[i] = 0;

	clks[CG_INFRA0] = ~idle_readl(INFRA_SW_CG_0_STA); /* INFRA0 */
	clks[CG_INFRA1] = ~idle_readl(INFRA_SW_CG_1_STA); /* INFRA1 */
	clks[CG_INFRA2] = ~idle_readl(INFRA_SW_CG_2_STA); /* INFRA2 */

	if (sys_is_on(SYS_DIS))
		clks[CG_DISP0] = ~idle_readl(DISP_CG_CON0); /* DISP */

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
						DIS_PWR_STA_MASK))
				return false;
		}
	} else if (mode == MT_SOIDLE) {
		if (!soidle_by_pass_pg) {
			if (sta & (MFG_PWR_STA_MASK |
						ISP_PWR_STA_MASK |
						VDE_PWR_STA_MASK |
						VEN_PWR_STA_MASK))
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

char pll_name[NR_PLLS][10] = {
	"UNIVPLL",
	"MMPLL",
	"MSDCPLL",
	"VENCPLL",
};

const char *pll_grp_get_name(int id)
{
	return pll_name[id];
}

int is_pll_on(int id)
{
	return idle_readl(APMIXEDSYS(0x230 + id * 0x10)) & 0x1;
}

bool pll_check_idle_can_enter(unsigned int *condition_mask, unsigned int *block_mask)
{
	int i, j;
	unsigned int pll_mask;

	memset(block_mask, 0, NR_PLLS * sizeof(unsigned int));

	for (i = 0; i < NR_PLLS; i++) {
		pll_mask = is_pll_on(i) & condition_mask[i];
		if (pll_mask) {
			for (j = 0; j < NR_PLLS; j++)
				block_mask[j] = is_pll_on(j) & condition_mask[j];

			return false;
		}
	}

	return true;
}

static int __init get_base_from_matching_node(
				     const struct of_device_id *ids, void __iomem **pbase, int idx, const char *cmp)
{
	struct device_node *node;

	node = of_find_matching_node(NULL, ids);
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
	static const struct of_device_id audiosys_ids[] = {
		{.compatible = "mediatek,audio"},
		{.compatible = "mediatek,mt6755-audiosys"},
		{ /* sentinel */ }
	};

	get_base_from_node("mediatek,infracfg_ao", &infrasys_base, 0);
	get_base_from_node("mediatek,mmsys_config", &mmsys_base, 0);
	get_base_from_node("mediatek,sleep", &sleepsys_base, 0);
	get_base_from_node("mediatek,topckgen", &topcksys_base, 0);
	get_base_from_node("mediatek,apmixed", &apmixed_base_in_idle, 0);
	get_base_from_node("mediatek,mt6755-mfgsys", &mfgsys_base, 0);
	get_base_from_node("mediatek,mt6755-imgsys", &imgsys_base, 0);
	get_base_from_node("mediatek,mt6755-vdecsys", &vdecsys_base, 0);
	get_base_from_node("mediatek,mt6755-vencsys", &vencsys_base, 0);

	get_base_from_matching_node(audiosys_ids, &audiosys_base_in_idle, 0, "audio");
}

const char *cg_grp_get_name(int id)
{
	BUG_ON(INVALID_GRP_ID(id));
	return cg_group_name[id];
}

bool is_disp_pwm_rosc(void)
{
	u32 sta = idle_readl(CLK_CFG_7) & 0x3;

	return (sta == 2) || (sta == 3);
}

bool is_auxadc_released(void)
{
#if 0
	if ((~idle_readl(INFRA_SW_CG_0_STA) & 0x400) == 0x400) {
		idle_err("AUXADC CG does not be released\n");
		return false;
	}
#endif

	return true;
}

bool vcore_dvfs_is_progressing(void)
{
	return false;
}

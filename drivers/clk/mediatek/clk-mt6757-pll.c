/*
 * Copyright (c) 2014 MediaTek Inc.
 * Author: James Liao <jamesjj.liao@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/io.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/clkdev.h>

#include "clk-mtk-v1.h"
#include "clk-pll-v1.h"
#include "clk-mt6757-pll.h"

#if !defined(MT_CCF_DEBUG) || !defined(MT_CCF_BRINGUP)
#define MT_CCF_DEBUG	0
#define MT_CCF_BRINGUP  1
#endif

#ifndef GENMASK
#define GENMASK(h, l)	(((U32_C(1) << ((h) - (l) + 1)) - 1) << (l))
#endif

/*
 * clk_pll
 */

#define PLL_BASE_EN	BIT(0)
#define PLL_PWR_ON	BIT(0)
#define PLL_ISO_EN	BIT(1)
#define PLL_PCW_CHG	BIT(31)
#define RST_BAR_MASK	BIT(24)
#define AUDPLL_TUNER_EN	BIT(31)
#define ULPOSC_EN BIT(0)
#define ULPOSC_RST BIT(1)
#define ULPOSC_CG_EN BIT(2)

static const u32 pll_posdiv_map[8] = { 1, 2, 4, 8, 16, 16, 16, 16 };

static u32 calc_pll_vco_freq(u32 fin, u32 pcw, u32 vcodivsel, u32 prediv, u32 pcwfbits)
{
	/* vco = (fin * pcw * vcodivsel / prediv) >> pcwfbits; */
	u64 vco = fin;
	u8 c;

	vco = vco * pcw * vcodivsel;
	do_div(vco, prediv);

	if (vco & GENMASK(pcwfbits - 1, 0))
		c = 1;

	vco >>= pcwfbits;

	if (c)
		++vco;

	return (u32) vco;
}

static u32 freq_limit(u32 freq)
{
	static const u64 freq_max = 3000UL * 1000 * 1000;	/* 3000 MHz */
	static const u32 freq_min = 1000 * 1000 * 1000 / 16;	/* 62.5 MHz */

	if (freq <= freq_min)
		freq = freq_min + 16;
	else if (freq > freq_max)
		freq = freq_max;

	return freq;
}

static int calc_pll_freq_cfg(u32 *pcw, u32 *postdiv_idx, u32 freq, u32 fin, int pcwfbits)
{
	static const u64 freq_max = 3000UL * 1000 * 1000;	/* 3000 MHz */
	static const u64 freq_min = 1000 * 1000 * 1000;	/* 1000 MHz */
	static const u64 postdiv[] = { 1, 2, 4, 8, 16 };
	u64 n_info;
	u32 idx;


	/* search suitable postdiv */
	for (idx = *postdiv_idx;
	     idx < ARRAY_SIZE(postdiv) && postdiv[idx] * freq <= freq_min; idx++)
		;

	if (idx >= ARRAY_SIZE(postdiv))
		return -EINVAL;	/* freq is out of range (too low) */
	else if (postdiv[idx] * freq > freq_max)
		return -EINVAL;	/* freq is out of range (too high) */

	/* n_info = freq * postdiv / 26MHz * 2^pcwfbits */
	n_info = (postdiv[idx] * freq) << pcwfbits;
	do_div(n_info, fin);

	*postdiv_idx = idx;
	*pcw = (u32) n_info;

	return 0;
}

static int clk_pll_is_enabled(struct clk_hw *hw)
{
	struct mtk_clk_pll *pll = to_mtk_clk_pll(hw);
	int r;
#if MT_CCF_BRINGUP
	pr_debug("[CCF] %s: %s: S\n", __func__, __clk_get_name(hw->clk));
	return 1;
#endif				/* MT_CCF_BRINGUP */

	r = (readl_relaxed(pll->base_addr) & PLL_BASE_EN) != 0;

#if MT_CCF_DEBUG
	pr_debug("[CCF] %s:%d: %s\n", __func__, r, __clk_get_name(hw->clk));
#endif				/* MT_CCF_DEBUG */

	return r;
}

static int clk_pll_enable(struct clk_hw *hw)
{
	unsigned long flags = 0;
	struct mtk_clk_pll *pll = to_mtk_clk_pll(hw);
	u32 r;

#if MT_CCF_DEBUG
	pr_debug("[CCF] %s: %s: S\n", __func__, __clk_get_name(hw->clk));
#endif				/* MT_CCF_DEBUG */
#if MT_CCF_BRINGUP
	return 0;
#endif				/* MT_CCF_BRINGUP */

	mtk_clk_lock(flags);

	r = readl_relaxed(pll->pwr_addr) | PLL_PWR_ON;
	writel_relaxed(r, pll->pwr_addr);
	wmb();			/* sync write before delay */
	udelay(1);

	r = readl_relaxed(pll->pwr_addr) & ~PLL_ISO_EN;
	writel_relaxed(r, pll->pwr_addr);
	wmb();			/* sync write before delay */
	udelay(1);

	r = readl_relaxed(pll->base_addr) | pll->en_mask;
	writel_relaxed(r, pll->base_addr);
	wmb();			/* sync write before delay */
	udelay(20);

	if (pll->flags & HAVE_RST_BAR) {
		r = readl_relaxed(pll->base_addr) | RST_BAR_MASK;
		writel_relaxed(r, pll->base_addr);
	}

	mtk_clk_unlock(flags);

	return 0;
}

static void clk_pll_disable(struct clk_hw *hw)
{
	unsigned long flags = 0;
	struct mtk_clk_pll *pll = to_mtk_clk_pll(hw);
	u32 r;

#if MT_CCF_DEBUG
	pr_debug("[CCF] %s: %s: flags=%u, PLL_AO: %d\n", __func__,
		 __clk_get_name(hw->clk), pll->flags, !!(pll->flags & PLL_AO));
#endif				/* MT_CCF_DEBUG */
#if MT_CCF_BRINGUP
	return;
#endif				/* MT_CCF_BRINGUP */

	if (pll->flags & PLL_AO)
		return;

	mtk_clk_lock(flags);

	if (pll->flags & HAVE_RST_BAR) {
		r = readl_relaxed(pll->base_addr) & ~RST_BAR_MASK;
		writel_relaxed(r, pll->base_addr);
	}

	r = readl_relaxed(pll->base_addr) & ~PLL_BASE_EN;
	writel_relaxed(r, pll->base_addr);

	r = readl_relaxed(pll->pwr_addr) | PLL_ISO_EN;
	writel_relaxed(r, pll->pwr_addr);

	r = readl_relaxed(pll->pwr_addr) & ~PLL_PWR_ON;
	writel_relaxed(r, pll->pwr_addr);

	mtk_clk_unlock(flags);
}

static long clk_pll_round_rate(struct clk_hw *hw, unsigned long rate, unsigned long *prate)
{
	u32 pcwfbits = 14;
	u32 pcw = 0;
	u32 postdiv = 0;
	u32 r;

#if MT_CCF_DEBUG
	pr_debug("[CCF] %s: %s: rate=%lu\n", __func__, __clk_get_name(hw->clk), rate);
#endif
#if MT_CCF_BRINGUP
	return 0;
#endif				/* MT_CCF_BRINGUP */

	*prate = *prate ? *prate : 26000000;
	rate = freq_limit(rate);
	calc_pll_freq_cfg(&pcw, &postdiv, rate, *prate, pcwfbits);

	r = calc_pll_vco_freq(*prate, pcw, 1, 1, pcwfbits);
	r = (r + pll_posdiv_map[postdiv] - 1) / pll_posdiv_map[postdiv];

	return r;
}

#define SDM_PLL_POSTDIV_H	6
#define SDM_PLL_POSTDIV_L	4
#define SDM_PLL_POSTDIV_MASK	GENMASK(SDM_PLL_POSTDIV_H, SDM_PLL_POSTDIV_L)
#define SDM_PLL_PCW_H		21
#define SDM_PLL_PCW_L		0
#define SDM_PLL_PCW_MASK	GENMASK(SDM_PLL_PCW_H, SDM_PLL_PCW_L)

static unsigned long clk_sdm_pll_recalc_rate(struct clk_hw *hw, unsigned long parent_rate)
{
	struct mtk_clk_pll *pll = to_mtk_clk_pll(hw);

	u32 con0 = readl_relaxed(pll->base_addr);
	u32 con1 = readl_relaxed(pll->base_addr + 4);

	u32 posdiv = (con0 & SDM_PLL_POSTDIV_MASK) >> SDM_PLL_POSTDIV_L;
	u32 pcw = (con1 & SDM_PLL_PCW_MASK) >> SDM_PLL_PCW_L;
	u32 pcwfbits = 14;

	u32 vco_freq;
	unsigned long r;

#if MT_CCF_BRINGUP
		pr_debug("[CCF] %s: %s: parent_rate=%lu\n", __func__, __clk_get_name(hw->clk), parent_rate);
		return 0;
#endif				/* MT_CCF_BRINGUP */

	parent_rate = parent_rate ? parent_rate : 26000000;

	vco_freq = calc_pll_vco_freq(parent_rate, pcw, 1, 1, pcwfbits);
	r = (vco_freq + pll_posdiv_map[posdiv] - 1) / pll_posdiv_map[posdiv];
#if MT_CCF_DEBUG
	pr_debug("[CCF] %s: %lu: %s\n", __func__, r, __clk_get_name(hw->clk));
#endif
	return r;
}

static void clk_sdm_pll_set_rate_regs(struct clk_hw *hw, u32 pcw, u32 postdiv_idx)
{
	unsigned long flags = 0;
	struct mtk_clk_pll *pll = to_mtk_clk_pll(hw);
	void __iomem *con0_addr = pll->base_addr;
	void __iomem *con1_addr = pll->base_addr + 4;
	u32 con0;
	u32 con1;
	u32 pll_en;

	mtk_clk_lock(flags);

	con0 = readl_relaxed(con0_addr);
	con1 = readl_relaxed(con1_addr);

	pll_en = con0 & PLL_BASE_EN;

	/* set postdiv */
	con0 &= ~SDM_PLL_POSTDIV_MASK;
	con0 |= postdiv_idx << SDM_PLL_POSTDIV_L;
	writel_relaxed(con0, con0_addr);

	/* set pcw */
	con1 &= ~SDM_PLL_PCW_MASK;
	con1 |= pcw << SDM_PLL_PCW_L;

	if (pll_en)
		con1 |= PLL_PCW_CHG;

	writel_relaxed(con1, con1_addr);

	if (pll_en) {
		wmb();		/* sync write before delay */
		udelay(20);
	}

	mtk_clk_unlock(flags);
}

static int clk_sdm_pll_set_rate(struct clk_hw *hw, unsigned long rate, unsigned long parent_rate)
{
	u32 pcwfbits = 14;
	u32 pcw = 0;
	u32 postdiv_idx = 0;
	int r;

#if MT_CCF_BRINGUP
	pr_debug("[CCF] %s: %s: S\n", __func__, __clk_get_name(hw->clk));
	return 0;
#endif				/* MT_CCF_BRINGUP */
	parent_rate = parent_rate ? parent_rate : 26000000;
	r = calc_pll_freq_cfg(&pcw, &postdiv_idx, rate, parent_rate, pcwfbits);
#if MT_CCF_DEBUG
	pr_debug("[CCF] %s: %s, rate: %lu, pcw: %u, postdiv_idx: %u\n",
		 __func__, __clk_get_name(hw->clk), rate, pcw, postdiv_idx);
#endif
	if (r == 0)
		clk_sdm_pll_set_rate_regs(hw, pcw, postdiv_idx);

	return r;
}

const struct clk_ops mt_clk_sdm_pll_ops = {
	.is_enabled = clk_pll_is_enabled,
	.enable = clk_pll_enable,
	.disable = clk_pll_disable,
	.recalc_rate = clk_sdm_pll_recalc_rate,
	.round_rate = clk_pll_round_rate,
	.set_rate = clk_sdm_pll_set_rate,
};

#define ARM_PLL_POSTDIV_H	26
#define ARM_PLL_POSTDIV_L	24
#define ARM_PLL_POSTDIV_MASK	GENMASK(ARM_PLL_POSTDIV_H, ARM_PLL_POSTDIV_L)
#define ARM_PLL_PCW_H		21
#define ARM_PLL_PCW_L		0
#define ARM_PLL_PCW_MASK	GENMASK(ARM_PLL_PCW_H, ARM_PLL_PCW_L)

static unsigned long clk_arm_pll_recalc_rate(struct clk_hw *hw, unsigned long parent_rate)
{
	struct mtk_clk_pll *pll = to_mtk_clk_pll(hw);

	u32 con1 = readl_relaxed(pll->base_addr + 4);

	u32 posdiv = (con1 & ARM_PLL_POSTDIV_MASK) >> ARM_PLL_POSTDIV_L;
	u32 pcw = (con1 & ARM_PLL_PCW_MASK) >> ARM_PLL_PCW_L;
	u32 pcwfbits = 14;

	u32 vco_freq;
	unsigned long r;

#if MT_CCF_BRINGUP
	pr_debug("[CCF] %s: %s: S\n", __func__, __clk_get_name(hw->clk));
	return 0;
#endif				/* MT_CCF_BRINGUP */
	parent_rate = parent_rate ? parent_rate : 26000000;

	vco_freq = calc_pll_vco_freq(parent_rate, pcw, 1, 1, pcwfbits);
	r = (vco_freq + pll_posdiv_map[posdiv] - 1) / pll_posdiv_map[posdiv];
#if MT_CCF_DEBUG
	pr_debug("[CCF] %s: %lu: %s\n", __func__, r, __clk_get_name(hw->clk));
#endif
	return r;
}

static void clk_arm_pll_set_rate_regs(struct clk_hw *hw, u32 pcw, u32 postdiv_idx)
{
	unsigned long flags = 0;
	struct mtk_clk_pll *pll = to_mtk_clk_pll(hw);
	void __iomem *con0_addr = pll->base_addr;
	void __iomem *con1_addr = pll->base_addr + 4;
	u32 con0;
	u32 con1;
	u32 pll_en;

	mtk_clk_lock(flags);

	con0 = readl_relaxed(con0_addr);
	con1 = readl_relaxed(con1_addr);

	pll_en = con0 & PLL_BASE_EN;

	/* postdiv */
	con1 &= ~ARM_PLL_POSTDIV_MASK;
	con1 |= postdiv_idx << ARM_PLL_POSTDIV_L;

	/* pcw */
	con1 &= ~ARM_PLL_PCW_MASK;
	con1 |= pcw << ARM_PLL_PCW_L;

	if (pll_en)
		con1 |= PLL_PCW_CHG;

	writel_relaxed(con1, con1_addr);

	if (pll_en) {
		wmb();		/* sync write before delay */
		udelay(20);
	}

	mtk_clk_unlock(flags);
}

static int clk_arm_pll_set_rate(struct clk_hw *hw, unsigned long rate, unsigned long parent_rate)
{
	u32 pcwfbits = 14;
	u32 pcw = 0;
	u32 postdiv_idx = 0;
	int r;

#if MT_CCF_BRINGUP
	pr_debug("[CCF] %s: %s: S\n", __func__, __clk_get_name(hw->clk));
	return 0;
#endif				/* MT_CCF_BRINGUP */
	parent_rate = parent_rate ? parent_rate : 26000000;
	r = calc_pll_freq_cfg(&pcw, &postdiv_idx, rate, parent_rate, pcwfbits);

#if MT_CCF_DEBUG
	pr_debug("[CCF] %s: %s, rate: %lu, pcw: %u, postdiv_idx: %u\n",
		 __func__, __clk_get_name(hw->clk), rate, pcw, postdiv_idx);
#endif
	if (r == 0)
		clk_arm_pll_set_rate_regs(hw, pcw, postdiv_idx);

	return r;
}

const struct clk_ops mt_clk_arm_pll_ops = {
	.is_enabled = clk_pll_is_enabled,
	.enable = clk_pll_enable,
	.disable = clk_pll_disable,
	.recalc_rate = clk_arm_pll_recalc_rate,
	.round_rate = clk_pll_round_rate,
	.set_rate = clk_arm_pll_set_rate,
};

static long clk_mm_pll_round_rate(struct clk_hw *hw, unsigned long rate, unsigned long *prate)
{
	u32 pcwfbits = 14;
	u32 pcw = 0;
	u32 postdiv = 0;
	u32 r;

#if MT_CCF_BRINGUP
	pr_debug("[CCF] %s: %s: S\n", __func__, __clk_get_name(hw->clk));
	return 0;
#endif				/* MT_CCF_BRINGUP */
#if MT_CCF_DEBUG
	pr_debug("%s, rate: %lu\n", __clk_get_name(hw->clk), rate);
#endif
	if (rate <= 702000000)
		postdiv = 2;

	*prate = *prate ? *prate : 26000000;
	rate = freq_limit(rate);
	calc_pll_freq_cfg(&pcw, &postdiv, rate, *prate, pcwfbits);

	r = calc_pll_vco_freq(*prate, pcw, 1, 1, pcwfbits);
	r = (r + pll_posdiv_map[postdiv] - 1) / pll_posdiv_map[postdiv];

	return r;
}

static int clk_mm_pll_set_rate(struct clk_hw *hw, unsigned long rate, unsigned long parent_rate)
{
	u32 pcwfbits = 14;
	u32 pcw = 0;
	u32 postdiv_idx = 0;
	int r;

#if MT_CCF_BRINGUP
	pr_debug("[CCF] %s: %s: S\n", __func__, __clk_get_name(hw->clk));
	return 0;
#endif				/* MT_CCF_BRINGUP */
	if (rate <= 702000000)
		postdiv_idx = 2;

	parent_rate = parent_rate ? parent_rate : 26000000;
	r = calc_pll_freq_cfg(&pcw, &postdiv_idx, rate, parent_rate, pcwfbits);
#if MT_CCF_DEBUG
	pr_debug("%s, rate: %lu, pcw: %u, postdiv_idx: %u\n",
		 __clk_get_name(hw->clk), rate, pcw, postdiv_idx);
#endif
	if (r == 0)
		clk_arm_pll_set_rate_regs(hw, pcw, postdiv_idx);

	return r;
}

const struct clk_ops mt_clk_mm_pll_ops = {
	.is_enabled = clk_pll_is_enabled,
	.enable = clk_pll_enable,
	.disable = clk_pll_disable,
	.recalc_rate = clk_arm_pll_recalc_rate,
	.round_rate = clk_mm_pll_round_rate,
	.set_rate = clk_mm_pll_set_rate,
};

static int clk_univ_pll_enable(struct clk_hw *hw)
{
	unsigned long flags = 0;
	struct mtk_clk_pll *pll = to_mtk_clk_pll(hw);
	u32 r;

#if MT_CCF_BRINGUP
	pr_debug("[CCF] %s: %s: S\n", __func__, __clk_get_name(hw->clk));
	return 0;
#endif				/* MT_CCF_BRINGUP */
#if MT_CCF_DEBUG
	pr_debug("%s\n", __clk_get_name(hw->clk));
#endif
	mtk_clk_lock(flags);

	r = readl_relaxed(pll->base_addr) | pll->en_mask;
	writel_relaxed(r, pll->base_addr);
	wmb();			/* sync write before delay */
	udelay(20);

	if (pll->flags & HAVE_RST_BAR) {
		r = readl_relaxed(pll->base_addr) | RST_BAR_MASK;
		writel_relaxed(r, pll->base_addr);
	}

	mtk_clk_unlock(flags);

	return 0;
}

static void clk_univ_pll_disable(struct clk_hw *hw)
{
	unsigned long flags = 0;
	struct mtk_clk_pll *pll = to_mtk_clk_pll(hw);
	u32 r;

#if MT_CCF_BRINGUP
	pr_debug("[CCF] %s: %s: S\n", __func__, __clk_get_name(hw->clk));
	return;
#endif				/* MT_CCF_BRINGUP */
#if MT_CCF_DEBUG
	pr_debug("%s: PLL_AO: %d\n", __clk_get_name(hw->clk), !!(pll->flags & PLL_AO));
#endif
	if (pll->flags & PLL_AO)
		return;

	mtk_clk_lock(flags);

	if (pll->flags & HAVE_RST_BAR) {
		r = readl_relaxed(pll->base_addr) & ~RST_BAR_MASK;
		writel_relaxed(r, pll->base_addr);
	}

	r = readl_relaxed(pll->base_addr) & ~PLL_BASE_EN;
	writel_relaxed(r, pll->base_addr);

	mtk_clk_unlock(flags);
}

#define UNIV_PLL_POSTDIV_H	6
#define UNIV_PLL_POSTDIV_L	4
#define UNIV_PLL_POSTDIV_MASK	GENMASK(UNIV_PLL_POSTDIV_H, UNIV_PLL_POSTDIV_L)
#define UNIV_PLL_FBKDIV_H	21
#define UNIV_PLL_FBKDIV_L	14
#define UNIV_PLL_FBKDIV_MASK	GENMASK(UNIV_PLL_FBKDIV_H, UNIV_PLL_FBKDIV_L)

static unsigned long clk_univ_pll_recalc_rate(struct clk_hw *hw, unsigned long parent_rate)
{
	struct mtk_clk_pll *pll = to_mtk_clk_pll(hw);

	u32 con0 = readl_relaxed(pll->base_addr);
	u32 con1 = readl_relaxed(pll->base_addr + 4);

	u32 fbkdiv = (con1 & UNIV_PLL_FBKDIV_MASK) >> UNIV_PLL_FBKDIV_L;
	u32 posdiv = (con0 & UNIV_PLL_POSTDIV_MASK) >> UNIV_PLL_POSTDIV_L;

	u32 vco_freq;
	unsigned long r;

#if MT_CCF_BRINGUP
		pr_debug("[CCF] %s: %s: S\n", __func__, __clk_get_name(hw->clk));
		return 0;
#endif				/* MT_CCF_BRINGUP */

	parent_rate = parent_rate ? parent_rate : 26000000;

	vco_freq = parent_rate * fbkdiv;
	r = (vco_freq + pll_posdiv_map[posdiv] - 1) / pll_posdiv_map[posdiv];
#if MT_CCF_DEBUG
	pr_debug("%lu: %s\n", r, __clk_get_name(hw->clk));
#endif
	return r;
}

static void clk_univ_pll_set_rate_regs(struct clk_hw *hw, u32 pcw, u32 postdiv_idx)
{
	unsigned long flags = 0;
	struct mtk_clk_pll *pll = to_mtk_clk_pll(hw);
	void __iomem *con0_addr = pll->base_addr;
	void __iomem *con1_addr = pll->base_addr + 4;
	u32 con0;
	u32 con1;
	u32 pll_en;

	mtk_clk_lock(flags);

	con0 = readl_relaxed(con0_addr);
	con1 = readl_relaxed(con1_addr);

	pll_en = con0 & PLL_BASE_EN;

	/* postdiv */
	con0 &= ~UNIV_PLL_POSTDIV_MASK;
	con0 |= postdiv_idx << UNIV_PLL_POSTDIV_L;

	/* fkbdiv */
	con1 &= ~UNIV_PLL_FBKDIV_MASK;
	con1 |= pcw << UNIV_PLL_FBKDIV_L;

	writel_relaxed(con0, con0_addr);
	writel_relaxed(con1, con1_addr);

	if (pll_en) {
		wmb();		/* sync write before delay */
		udelay(20);
	}

	mtk_clk_unlock(flags);
}

static long clk_univ_pll_round_rate(struct clk_hw *hw, unsigned long rate, unsigned long *prate)
{
	u32 pcwfbits = 0;
	u32 pcw = 0;
	u32 postdiv = 0;
	u32 r;

#if MT_CCF_BRINGUP
	pr_debug("[CCF] %s: %s: S\n", __func__, __clk_get_name(hw->clk));
	return 0;
#endif				/* MT_CCF_BRINGUP */
#if MT_CCF_DEBUG
	pr_debug("%s, rate: %lu\n", __clk_get_name(hw->clk), rate);
#endif
	*prate = *prate ? *prate : 26000000;
	rate = freq_limit(rate);
	calc_pll_freq_cfg(&pcw, &postdiv, rate, *prate, pcwfbits);

	r = *prate * pcw;
	r = (r + pll_posdiv_map[postdiv] - 1) / pll_posdiv_map[postdiv];

	return r;
}

static int clk_univ_pll_set_rate(struct clk_hw *hw, unsigned long rate, unsigned long parent_rate)
{
	u32 pcwfbits = 0;
	u32 pcw = 0;
	u32 postdiv_idx = 0;
	int r;

#if MT_CCF_BRINGUP
	pr_debug("[CCF] %s: %s: S\n", __func__, __clk_get_name(hw->clk));
	return 0;
#endif				/* MT_CCF_BRINGUP */
	parent_rate = parent_rate ? parent_rate : 26000000;
	r = calc_pll_freq_cfg(&pcw, &postdiv_idx, rate, parent_rate, pcwfbits);
#if MT_CCF_DEBUG
	pr_debug("%s, rate: %lu, pcw: %u, postdiv_idx: %u\n",
		 __clk_get_name(hw->clk), rate, pcw, postdiv_idx);
#endif
	if (r == 0)
		clk_univ_pll_set_rate_regs(hw, pcw, postdiv_idx);

	return r;
}

const struct clk_ops mt_clk_univ_pll_ops = {
	.is_enabled = clk_pll_is_enabled,
	.enable = clk_univ_pll_enable,
	.disable = clk_univ_pll_disable,
	.recalc_rate = clk_univ_pll_recalc_rate,
	.round_rate = clk_univ_pll_round_rate,
	.set_rate = clk_univ_pll_set_rate,
};

static int clk_aud_pll_enable(struct clk_hw *hw)
{
	unsigned long flags = 0;
	struct mtk_clk_pll *pll = to_mtk_clk_pll(hw);
	u32 r;

#if MT_CCF_BRINGUP
	pr_debug("[CCF] %s: %s: S\n", __func__, __clk_get_name(hw->clk));
	return 0;
#endif				/* MT_CCF_BRINGUP */
#if MT_CCF_DEBUG
	pr_debug("%s\n", __clk_get_name(hw->clk));
#endif
	mtk_clk_lock(flags);

	r = readl_relaxed(pll->pwr_addr) | PLL_PWR_ON;
	writel_relaxed(r, pll->pwr_addr);
	wmb();			/* sync write before delay */
	udelay(1);

	r = readl_relaxed(pll->pwr_addr) & ~PLL_ISO_EN;
	writel_relaxed(r, pll->pwr_addr);
	wmb();			/* sync write before delay */
	udelay(1);

	r = readl_relaxed(pll->base_addr) | pll->en_mask;
	writel_relaxed(r, pll->base_addr);

	r = readl_relaxed(pll->base_addr + 16) | AUDPLL_TUNER_EN;
	writel_relaxed(r, pll->base_addr + 16);
	wmb();			/* sync write before delay */
	udelay(20);

	if (pll->flags & HAVE_RST_BAR) {
		r = readl_relaxed(pll->base_addr) | RST_BAR_MASK;
		writel_relaxed(r, pll->base_addr);
	}

	mtk_clk_unlock(flags);

	return 0;
}

static void clk_aud_pll_disable(struct clk_hw *hw)
{
	unsigned long flags = 0;
	struct mtk_clk_pll *pll = to_mtk_clk_pll(hw);
	u32 r;

#if MT_CCF_BRINGUP
	pr_debug("[CCF] %s: %s: S\n", __func__, __clk_get_name(hw->clk));
	return;
#endif				/* MT_CCF_BRINGUP */
#if MT_CCF_DEBUG
	pr_debug("%s: PLL_AO: %d\n", __clk_get_name(hw->clk), !!(pll->flags & PLL_AO));
#endif
	if (pll->flags & PLL_AO)
		return;

	mtk_clk_lock(flags);

	if (pll->flags & HAVE_RST_BAR) {
		r = readl_relaxed(pll->base_addr) & ~RST_BAR_MASK;
		writel_relaxed(r, pll->base_addr);
	}

	r = readl_relaxed(pll->base_addr + 8) & ~AUDPLL_TUNER_EN;
	writel_relaxed(r, pll->base_addr + 8);

	r = readl_relaxed(pll->base_addr) & ~PLL_BASE_EN;
	writel_relaxed(r, pll->base_addr);

	r = readl_relaxed(pll->pwr_addr) | PLL_ISO_EN;
	writel_relaxed(r, pll->pwr_addr);

	r = readl_relaxed(pll->pwr_addr) & ~PLL_PWR_ON;
	writel_relaxed(r, pll->pwr_addr);

	mtk_clk_unlock(flags);
}

#define AUD_PLL_POSTDIV_H	6
#define AUD_PLL_POSTDIV_L	4
#define AUD_PLL_POSTDIV_MASK	GENMASK(AUD_PLL_POSTDIV_H, AUD_PLL_POSTDIV_L)
#define AUD_PLL_PCW_H		31
#define AUD_PLL_PCW_L		0
#define AUD_PLL_PCW_MASK	GENMASK(AUD_PLL_PCW_H, AUD_PLL_PCW_L)

static unsigned long clk_aud_pll_recalc_rate(struct clk_hw *hw, unsigned long parent_rate)
{
	struct mtk_clk_pll *pll = to_mtk_clk_pll(hw);

	u32 con0 = readl_relaxed(pll->base_addr);
	u32 con1 = readl_relaxed(pll->base_addr + 4);

	u32 posdiv = (con0 & AUD_PLL_POSTDIV_MASK) >> AUD_PLL_POSTDIV_L;
	u32 pcw = (con1 & AUD_PLL_PCW_MASK) >> AUD_PLL_PCW_L;
	u32 pcwfbits = 24;

	u32 vco_freq;
	unsigned long r;

#if MT_CCF_BRINGUP
		pr_debug("[CCF] %s: %s: S\n", __func__, __clk_get_name(hw->clk));
		return 0;
#endif				/* MT_CCF_BRINGUP */

	parent_rate = parent_rate ? parent_rate : 26000000;

	vco_freq = calc_pll_vco_freq(parent_rate, pcw, 1, 1, pcwfbits);
	r = (vco_freq + pll_posdiv_map[posdiv] - 1) / pll_posdiv_map[posdiv];
#if MT_CCF_DEBUG
	pr_debug("%lu: %s\n", r, __clk_get_name(hw->clk));
#endif
	return r;
}

static void clk_aud_pll_set_rate_regs(struct clk_hw *hw, u32 pcw, u32 postdiv_idx)
{
	unsigned long flags = 0;
	struct mtk_clk_pll *pll = to_mtk_clk_pll(hw);
	void __iomem *con0_addr = pll->base_addr;
	void __iomem *con1_addr = pll->base_addr + 4;
	void __iomem *con2_addr = pll->base_addr + 8;
	u32 con0;
	u32 con1;
	u32 pll_en;

	mtk_clk_lock(flags);

	con0 = readl_relaxed(con0_addr);
	con1 = readl_relaxed(con1_addr);

	pll_en = con0 & PLL_BASE_EN;

	/* set postdiv */
	con0 &= ~AUD_PLL_POSTDIV_MASK;
	con0 |= postdiv_idx << AUD_PLL_POSTDIV_L;
	writel_relaxed(con0, con0_addr);

	/* set pcw */
	con1 &= ~AUD_PLL_PCW_MASK;
	con1 |= pcw << AUD_PLL_PCW_L;

	if (pll_en)
		con0 |= PLL_PCW_CHG; /* aud PLL_PCW_CHG move from con1[31] to con0[31] */

	writel_relaxed(con1, con1_addr);
	writel_relaxed(con0, con0_addr); /* aud PLL_PCW_CHG move from con1[31] to con0[31] */
	writel_relaxed(con1 + 1, con2_addr);

	if (pll_en) {
		wmb();		/* sync write before delay */
		udelay(20);
	}

	mtk_clk_unlock(flags);
}

static long clk_aud_pll_round_rate(struct clk_hw *hw, unsigned long rate, unsigned long *prate)
{
	u32 pcwfbits = 24;
	u32 pcw = 0;
	u32 postdiv = 0;
	u32 r;

#if MT_CCF_BRINGUP
	pr_debug("[CCF] %s: %s: S\n", __func__, __clk_get_name(hw->clk));
	return 0;
#endif				/* MT_CCF_BRINGUP */
#if MT_CCF_DEBUG
	pr_debug("%s, rate: %lu\n", __clk_get_name(hw->clk), rate);
#endif
	*prate = *prate ? *prate : 26000000;
	rate = freq_limit(rate);
	calc_pll_freq_cfg(&pcw, &postdiv, rate, *prate, pcwfbits);

	r = calc_pll_vco_freq(*prate, pcw, 1, 1, pcwfbits);
	r = (r + pll_posdiv_map[postdiv] - 1) / pll_posdiv_map[postdiv];

	return r;
}

static int clk_aud_pll_set_rate(struct clk_hw *hw, unsigned long rate, unsigned long parent_rate)
{
	u32 pcwfbits = 24;
	u32 pcw = 0;
	u32 postdiv_idx = 0;
	int r;

#if MT_CCF_BRINGUP
	pr_debug("[CCF] %s: %s: rate=%lu, parent_rate=%lu\n", __func__,
		 __clk_get_name(hw->clk), rate, parent_rate);
	return 0;
#endif				/* MT_CCF_BRINGUP */
	parent_rate = parent_rate ? parent_rate : 26000000;
	r = calc_pll_freq_cfg(&pcw, &postdiv_idx, rate, parent_rate, pcwfbits);
#if MT_CCF_DEBUG
	pr_debug("%s, rate: %lu, pcw: %u, postdiv_idx: %u\n",
		 __clk_get_name(hw->clk), rate, pcw, postdiv_idx);
#endif
	if (r == 0)
		clk_aud_pll_set_rate_regs(hw, pcw, postdiv_idx);

	return r;
}

const struct clk_ops mt_clk_aud_pll_ops = {
	.is_enabled = clk_pll_is_enabled,
	.enable = clk_aud_pll_enable,
	.disable = clk_aud_pll_disable,
	.recalc_rate = clk_aud_pll_recalc_rate,
	.round_rate = clk_aud_pll_round_rate,
	.set_rate = clk_aud_pll_set_rate,
};

static int clk_spm_pll_enable(struct clk_hw *hw)
{
	unsigned long flags = 0;
	struct mtk_clk_pll *pll = to_mtk_clk_pll(hw);
	u32 r;

#if MT_CCF_BRINGUP
	pr_debug("[CCF] %s: %s: S\n", __func__, __clk_get_name(hw->clk));
	return 0;
#endif /* MT_CCF_BRINGUP */
/*	pr_debug("%s\n", __clk_get_name(hw->clk));*/

	mtk_clk_lock(flags);
/* OSC EN = 1 */
	r = readl_relaxed(pll->base_addr) | ULPOSC_EN;
	writel_relaxed(r, pll->base_addr);
	wmb();	/* sync write before delay */
	udelay(11);
/* OSC RST  */
	r = readl_relaxed(pll->base_addr) | ULPOSC_RST;
	writel_relaxed(r, pll->base_addr);
	wmb();	/* sync write before delay */
	udelay(40);
	r = readl_relaxed(pll->base_addr) & ~ULPOSC_RST;
	writel_relaxed(r, pll->base_addr);
	wmb();	/* sync write before delay */
	udelay(130);
/* OSC CG_EN = 1 */
	r = readl_relaxed(pll->base_addr) | ULPOSC_CG_EN;
	writel_relaxed(r, pll->base_addr);

	mtk_clk_unlock(flags);

	return 0;
}

static void clk_spm_pll_disable(struct clk_hw *hw)
{
	unsigned long flags = 0;
	struct mtk_clk_pll *pll = to_mtk_clk_pll(hw);
	u32 r;

#if MT_CCF_BRINGUP
	pr_debug("[CCF] %s: %s: S\n", __func__, __clk_get_name(hw->clk));
	return;
#endif /* MT_CCF_BRINGUP */
/*	pr_debug("%s: PLL_AO: %d\n",
		__clk_get_name(hw->clk), !!(pll->flags & PLL_AO));*/

	if (pll->flags & PLL_AO)
		return;

	mtk_clk_lock(flags);

/* OSC CG_EN = 0 */
	r = readl_relaxed(pll->base_addr) & ~ULPOSC_CG_EN;
	writel_relaxed(r, pll->base_addr);
	wmb();	/* sync write before delay */
	udelay(40);
/* OSC EN = 0 */
	r = readl_relaxed(pll->base_addr) & ~ULPOSC_EN;
	writel_relaxed(r, pll->base_addr);

	mtk_clk_unlock(flags);
}



static unsigned long clk_spm_pll_recalc_rate(
		struct clk_hw *hw,
		unsigned long parent_rate)
{
#if MT_CCF_BRINGUP
	pr_debug("[CCF] %s: %s: S\n", __func__, __clk_get_name(hw->clk));
	return 0;
#endif /* MT_CCF_BRINGUP */

	return 208000000;
}


static long clk_spm_pll_round_rate(
		struct clk_hw *hw,
		unsigned long rate,
		unsigned long *prate)
{

#if MT_CCF_BRINGUP
	pr_debug("[CCF] %s: %s: S\n", __func__, __clk_get_name(hw->clk));
	return 0;
#endif /* MT_CCF_BRINGUP */
/*	pr_debug("%s, rate: %lu\n", __clk_get_name(hw->clk), rate);*/

	return 208000000;
}

static int clk_spm_pll_set_rate(
		struct clk_hw *hw,
		unsigned long rate,
		unsigned long parent_rate)
{


#if MT_CCF_BRINGUP
	pr_debug("[CCF] %s: %s: rate=%lu, parent_rate=%lu\n", __func__,
		 __clk_get_name(hw->clk), rate, parent_rate);
	return 0;
#endif /* MT_CCF_BRINGUP */

	return 208000000;
}

const struct clk_ops mt_clk_spm_pll_ops = {
	.is_enabled	= clk_pll_is_enabled,
	.enable		= clk_spm_pll_enable,
	.disable	= clk_spm_pll_disable,
	.recalc_rate	= clk_spm_pll_recalc_rate,
	.round_rate	= clk_spm_pll_round_rate,
	.set_rate	= clk_spm_pll_set_rate,
};


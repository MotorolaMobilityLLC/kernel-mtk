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

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <dt-bindings/clock/mt6757-clk.h>
#include "clk-gate.h"
#include "clk-mtk-v1.h"
#include "clk-pll-v1.h"
/* #include "clk-gate-v1.h" */
#include "clk-mux.h"
#include "clk-mt6757-pll.h"


/* ====================common-start======================= */

#if !defined(MT_CCF_DEBUG) \
		|| !defined(CLK_DEBUG) || !defined(DUMMY_REG_TEST)
#define MT_CCF_DEBUG	0
#define CONTROL_LIMIT	0
#define CLK_DEBUG	0
#define DUMMY_REG_TEST	0
#ifdef CONFIG_FPGA_EARLY_PORTING
#define MT_CCF_BRINGUP	0
#else
#define MT_CCF_BRINGUP	0
#endif
#endif


#ifdef CONFIG_ARM64
#define IOMEM(a)	((void __force __iomem *)((a)))
#endif

#define mt_reg_sync_writel(v, a) \
	do { \
		__raw_writel((v), IOMEM(a)); \
		mb(); } \
while (0)

#define clk_readl(addr)			__raw_readl(IOMEM(addr))

#define clk_writel(addr, val)   \
	mt_reg_sync_writel(val, addr)

#define clk_setl(addr, val) \
	mt_reg_sync_writel(clk_readl(addr) | (val), addr)

#define clk_clrl(addr, val) \
	mt_reg_sync_writel(clk_readl(addr) & ~(val), addr)

static DEFINE_SPINLOCK(mt6757_clk_lock);

#define to_mtk_clk_gate(_hw) container_of(_hw, struct mtk_clk_gate, hw)

#define CLK_GATE_INVERSE	BIT(0)
#define CLK_GATE_NO_SETCLR_REG	BIT(1)

/* ====================common-end======================= */

struct mtk_gate_regs {
	u32 sta_ofs;
	u32 clr_ofs;
	u32 set_ofs;
};

struct mtk_gate {
	int id;
	const char *name;
	const char *parent_name;
	const struct mtk_gate_regs *regs;
	int shift;
	uint32_t flags;
	const struct clk_ops *ops;
};

#define GATE(_id, _name, _parent, _regs, _shift, _flags) {	\
		.id = _id,					\
		.name = _name,					\
		.parent_name = _parent,				\
		.regs = &_regs,					\
		.shift = _shift,				\
		.flags = _flags,				\
		.ops = &mtk_clk_gate_ops_setclr,                \
}

/*
 * platform clocks
 */
/* ====================auto-start======================= */
/* ROOT */
#define clk_null		"clk_null"
#define clk26m		"clk26m"
#define clk32k		"clk32k"
#define clk13m		"clk13m"

/* TOP */
#define f26m_sel		"f26m_sel"
#define f32k_sel		"f32k_sel"
#define axi_sel		"axi_sel"
#define mm_sel		"mm_sel"
#define vdec_sel		"vdec_sel"
#define venc_sel		"venc_sel"
#define mfg_sel		"mfg_sel"
#define fg_sel		"fg_sel"
#define camtg_sel		"camtg_sel"
#define uart_sel		"uart_sel"
#define spi_sel		"spi_sel"
#define img_sel "img_sel"
#define msdc50_0_hclk_sel		"msdc50_0_hclk_sel"
#define msdc50_0_sel		"msdc50_0_sel"
#define msdc30_1_sel		"msdc30_1_sel"
#define msdc30_2_sel		"msdc30_2_sel"
#define msdc30_3_sel		"msdc30_3_sel"
#define audio_sel		"audio_sel"
#define aud_intbus_sel		"aud_intbus_sel"
#define pmicspi_sel		"pmicspi_sel"
#define atb_sel		"atb_sel"
#define dpi0_sel		"dpi0_sel"
#define scam_sel		"scam_sel"
#define aud_1_sel		"aud_1_sel"
#define aud_2_sel		"aud_2_sel"
#define disp_pwm_sel		"disp_pwm_sel"
#define ssusb_top_sys_sel		"ssusb_top_sys_sel"
#define ssusb_top_xhci_sel		"ssusb_top_xhci_sel"
#define usb_top_sel		"usb_top_sel"
#define spm_sel		"spm_sel"
#define bsi_spi_sel		"bsi_spi_sel"
#define i2c_sel		"i2c_sel"
#define dvfsp_sel		"dvfsp_sel"
#define f52m_mfg_sel	"f52m_mfg_sel"

/* DIV */
#define ad_armpll_l_ck_vrpoc_l		"ad_armpll_l_ck_vrpoc_l"
#define ad_armpll_ll_ck_vrpoc_ll		"ad_armpll_ll_ck_vrpoc_ll"
#define ad_ccipll_ck_vrpoc_cci		"ad_ccipll_ck_vrpoc_cci"
#define syspll_ck		"syspll_ck"
#define syspll_d2		"syspll_d2"
#define syspll1_d2		"syspll1_d2"
#define syspll1_d4		"syspll1_d4"
#define syspll1_d8		"syspll1_d8"
#define syspll1_d16		"syspll1_d16"
#define syspll_d3		"syspll_d3"
#define syspll2_d2		"syspll2_d2"
#define syspll2_d4		"syspll2_d4"
#define syspll2_d8		"syspll2_d8"
#define syspll_d3_d3		"syspll_d3_d3"
#define syspll_d5		"syspll_d5"
#define syspll3_d2		"syspll3_d2"
#define syspll3_d4		"syspll3_d4"
#define syspll_d7		"syspll_d7"
#define syspll4_d2		"syspll4_d2"
#define syspll4_d4		"syspll4_d4"
#define univpll_ck		"univpll_ck"
#define univpll_d2		"univpll_d2"
#define univpll1_d2		"univpll1_d2"
#define univpll1_d4		"univpll1_d4"
#define univpll1_d8		"univpll1_d8"
#define univpll_d3		"univpll_d3"
#define univpll2_d2		"univpll2_d2"
#define univpll2_d4		"univpll2_d4"
#define univpll2_d8		"univpll2_d8"
#define univpll_d5		"univpll_d5"
#define univpll3_d2		"univpll3_d2"
#define univpll3_d4		"univpll3_d4"
#define univpll3_d8		"univpll3_d8"
#define univpll_d7		"univpll_d7"
#define univ_192m_ck	"univ_192m_ck"
#define univpll_192m_d2	"univpll_192m_d2"
#define univpll_192m_d4	"univpll_192m_d4"
#define apll1_ck		"apll1_ck"
#define apll2_ck		"apll2_ck"
#define tvdpll_ck		"tvdpll_ck"
#define tvdpll_d2		"tvdpll_d2"
#define tvdpll_d4		"tvdpll_d4"
#define tvdpll_d8		"tvdpll_d8"
#define tvdpll_d16		"tvdpll_d16"
#define msdcpll_ck		"msdcpll_ck"
#define msdcpll_d2		"msdcpll_d2"
#define msdcpll_d4		"msdcpll_d4"
#define msdcpll_d8		"msdcpll_d8"
#define msdcpll_d16		"msdcpll_d16"
#define ad_osc_ck		"ad_osc_ck"
#define osc_d2		"osc_d2"
#define osc_d4		"osc_d4"
#define osc_d8		"osc_d8"
#define vencpll_ck "vencpll_ck"
#define mmpll_ck	"mmpll_ck"

/* APMIXED_SYS */
#define armpll_ll		"armpll_ll"
#define armpll_l		"armpll_l"
#define ccipll		"ccipll"
#define mainpll		"mainpll"
#define univpll		"univpll"
#define msdcpll		"msdcpll"
#define vencpll		"vencpll"
#define mmpll		"mmpll"
#define tvdpll		"tvdpll"
#define apll1		"apll1"
#define apll2		"apll2"
#define oscpll	"oscpll"

#define CLK_CFG_UPDATE 0x004
#define CLK_CFG_UPDATE1 0x008
#define CLK_CFG_0 0x040
#define CLK_CFG_0_SET 0x044
#define CLK_CFG_0_CLR 0x048
#define CLK_CFG_1 0x050
#define CLK_CFG_1_SET 0x054
#define CLK_CFG_1_CLR 0x058
#define CLK_CFG_2 0x060
#define CLK_CFG_2_SET 0x064
#define CLK_CFG_2_CLR 0x068
#define CLK_CFG_3 0x070
#define CLK_CFG_3_SET 0x074
#define CLK_CFG_3_CLR 0x078
#define CLK_CFG_4 0x080
#define CLK_CFG_4_SET 0x084
#define CLK_CFG_4_CLR 0x088
#define CLK_CFG_5 0x090
#define CLK_CFG_5_SET 0x094
#define CLK_CFG_5_CLR 0x098
#define CLK_CFG_6 0x0a0
#define CLK_CFG_6_SET 0x0a4
#define CLK_CFG_6_CLR 0x0a8
#define CLK_CFG_7 0x0b0
#define CLK_CFG_7_SET 0x0b4
#define CLK_CFG_7_CLR 0x0b8
#define CLK_CFG_8 0x0c0
#define CLK_CFG_8_SET 0x0c4
#define CLK_CFG_8_CLR 0x0c8
#define CLK_CFG_9 0x0d0
#define CLK_CFG_9_SET 0x0d4
#define CLK_CFG_9_CLR 0x0d8

struct mtk_fixed_factor {
	int id;
	const char *name;
	const char *parent_name;
	int mult;
	int div;
};

#define FACTOR(_id, _name, _parent, _mult, _div) { \
	.id = _id, \
	.name = _name, \
	.parent_name = _parent, \
	.mult = _mult, \
	.div = _div, \
}

static const struct mtk_fixed_factor top_divs[] __initconst = {
	FACTOR(TOP_AD_ARMPLL_L_CK_VRPOC_L, ad_armpll_l_ck_vrpoc_l, armpll_l, 1, 1),
	FACTOR(TOP_AD_ARMPLL_LL_CK_VRPOC_LL, ad_armpll_ll_ck_vrpoc_ll, armpll_ll, 1, 1),
	FACTOR(TOP_AD_CCIPLL_CK_VRPOC_CCI, ad_ccipll_ck_vrpoc_cci, ccipll, 1, 1),
	FACTOR(TOP_SYSPLL_CK, syspll_ck, mainpll, 1, 1),
	FACTOR(TOP_SYSPLL_D2, syspll_d2, syspll_ck, 1, 2),
	FACTOR(TOP_SYSPLL1_D2, syspll1_d2, syspll_d2, 1, 2),
	FACTOR(TOP_SYSPLL1_D4, syspll1_d4, syspll_d2, 1, 4),
	FACTOR(TOP_SYSPLL1_D8, syspll1_d8, syspll_d2, 1, 8),
	FACTOR(TOP_SYSPLL1_D16, syspll1_d16, syspll_d2, 1, 16),
	FACTOR(TOP_SYSPLL_D3, syspll_d3, syspll_ck, 1, 3),
	FACTOR(TOP_SYSPLL2_D2, syspll2_d2, syspll_d3, 1, 2),
	FACTOR(TOP_SYSPLL2_D4, syspll2_d4, syspll_d3, 1, 4),
	FACTOR(TOP_SYSPLL2_D8, syspll2_d8, syspll_d3, 1, 8),
	FACTOR(TOP_SYSPLL_D3_D3, syspll_d3_d3, syspll_d3, 1, 3),
	FACTOR(TOP_SYSPLL_D5, syspll_d5, syspll_ck, 1, 5),
	FACTOR(TOP_SYSPLL3_D2, syspll3_d2, syspll_d5, 1, 2),
	FACTOR(TOP_SYSPLL3_D4, syspll3_d4, syspll_d5, 1, 4),
	FACTOR(TOP_SYSPLL_D7, syspll_d7, syspll_ck, 1, 7),
	FACTOR(TOP_SYSPLL4_D2, syspll4_d2, syspll_d7, 1, 2),
	FACTOR(TOP_SYSPLL4_D4, syspll4_d4, syspll_d7, 1, 4),
	FACTOR(TOP_UNIVPLL_CK, univpll_ck, univpll, 1, 2),
	FACTOR(TOP_UNIVPLL_D2, univpll_d2, univpll_ck, 1, 2),
	FACTOR(TOP_UNIVPLL1_D2, univpll1_d2, univpll_d2, 1, 2),
	FACTOR(TOP_UNIVPLL1_D4, univpll1_d4, univpll_d2, 1, 4),
	FACTOR(TOP_UNIVPLL1_D8, univpll1_d8, univpll_d2, 1, 8),
	FACTOR(TOP_UNIVPLL_D3, univpll_d3, univpll_ck, 1, 3),
	FACTOR(TOP_UNIVPLL2_D2, univpll2_d2, univpll_d3, 1, 2),
	FACTOR(TOP_UNIVPLL2_D4, univpll2_d4, univpll_d3, 1, 4),
	FACTOR(TOP_UNIVPLL2_D8, univpll2_d8, univpll_d3, 1, 8),
	FACTOR(TOP_UNIVPLL_D5, univpll_d5, univpll_ck, 1, 5),
	FACTOR(TOP_UNIVPLL3_D2, univpll3_d2, univpll_d5, 1, 2),
	FACTOR(TOP_UNIVPLL3_D4, univpll3_d4, univpll_d5, 1, 4),
	FACTOR(TOP_UNIVPLL3_D8, univpll3_d8, univpll_d5, 1, 8),
	FACTOR(TOP_UNIVPLL_D7, univpll_d7, univpll_ck, 1, 7),
	FACTOR(TOP_UNIV_192M_CK, univ_192m_ck, univpll, 1, 13),
	FACTOR(TOP_UNIVPLL_192M_D2, univpll_192m_d2, univ_192m_ck, 1, 2),
	FACTOR(TOP_UNIVPLL_192M_D4, univpll_192m_d4, univ_192m_ck, 1, 4),
	FACTOR(TOP_APLL1_CK, apll1_ck, apll1, 1, 1),
	FACTOR(TOP_APLL2_CK, apll2_ck, apll2, 1, 1),
	FACTOR(TOP_TVDPLL_CK, tvdpll_ck, tvdpll, 1, 1),
	FACTOR(TOP_TVDPLL_D2, tvdpll_d2, tvdpll_ck, 1, 2),
	FACTOR(TOP_TVDPLL_D4, tvdpll_d4, tvdpll_ck, 1, 4),
	FACTOR(TOP_TVDPLL_D8, tvdpll_d8, tvdpll_ck, 1, 8),
	FACTOR(TOP_TVDPLL_D16, tvdpll_d16, tvdpll_ck, 1, 16),
	FACTOR(TOP_MSDCPLL_CK, msdcpll_ck, msdcpll, 1, 1),
	FACTOR(TOP_MSDCPLL_D2, msdcpll_d2, msdcpll_ck, 1, 2),
	FACTOR(TOP_MSDCPLL_D4, msdcpll_d4, msdcpll_ck, 1, 4),
	FACTOR(TOP_MSDCPLL_D8, msdcpll_d8, msdcpll_ck, 1, 8),
	FACTOR(TOP_MSDCPLL_D16, msdcpll_d16, msdcpll_ck, 1, 16),
	FACTOR(TOP_AD_OSC_CK, ad_osc_ck, oscpll, 1, 1),
	FACTOR(TOP_OSC_D2, osc_d2, ad_osc_ck, 1, 2),
	FACTOR(TOP_OSC_D4, osc_d4, ad_osc_ck, 1, 4),
	FACTOR(TOP_OSC_D8, osc_d8, ad_osc_ck, 1, 8),
	FACTOR(TOP_VENCPLL_CK, vencpll_ck, vencpll, 1, 1),
	FACTOR(TOP_MMPLL_CK, mmpll_ck, mmpll, 1, 1),
	FACTOR(INFRACFG_AO_CLK_13M, clk13m, clk26m, 1, 2),
};

static const char *const const axi_parents[] __initconst = {
	clk26m,
	syspll1_d4,
	syspll_d7,
	osc_d8
};

static const char *const mm_parents[] __initconst = {
	clk26m,
	syspll2_d2,
	vencpll_ck,
	syspll1_d2,
	univpll1_d2,
	syspll_d3,
	syspll_d2
};

static const char *const vdec_parents[] __initconst = {
	clk26m,
	vencpll_ck,
	syspll2_d2,
	syspll1_d2,
	syspll1_d4,
	univpll_d3,
	syspll_d5
};

static const char *const mfg_parents[] __initconst = {
	clk26m,
	mmpll_ck,
	univpll_d3,
	syspll_d3
};

static const char *const f52m_mfg_parents[] __initconst = {
	clk26m,
	univpll2_d2,
	univpll2_d4,
	univpll2_d8
};

static const char *const camtg_parents[] __initconst = {
	clk26m,
	univpll_192m_d4,/*univpll_d26*/
	univpll2_d2
};

static const char *const uart_parents[] __initconst = {
	clk26m,
	univpll2_d8
};

static const char *const spi_parents[] __initconst = {
	clk26m,
	syspll3_d2,
	syspll2_d4,
	msdcpll_d4
};

static const char *const msdc50_0_hclk_parents[] __initconst = {
	clk26m,
	syspll1_d2,
	syspll2_d2,
	syspll4_d2
};

static const char *const msdc50_0_parents[] __initconst = {
	clk26m,
	msdcpll_ck,
	msdcpll_d2,
	univpll1_d4,
	syspll2_d2,
	syspll_d7,
	msdcpll_d4,
	univpll_d2,
	univpll1_d2
};

static const char *const msdc30_1_parents[] __initconst = {
	clk26m,
	univpll2_d2,
	msdcpll_d4,
	univpll1_d4,
	syspll2_d2,
	syspll_d7,
	univpll_d7,
	msdcpll_d2
};

static const char *const msdc30_2_parents[] __initconst = {
	clk26m,
	univpll2_d2,
	msdcpll_d4,
	univpll1_d4,
	syspll2_d2,
	syspll_d7,
	univpll_d7,
	msdcpll_d2
};

static const char *const msdc30_3_parents[] __initconst = {
	clk26m,
	msdcpll_d8,
	msdcpll_d4,
	univpll1_d4,
	univpll_192m_d4,/*univpll_d26*/
	syspll_d7,
	univpll_d7,
	syspll3_d4,
	msdcpll_d16
};

static const char *const audio_parents[] __initconst = {
	clk26m,
	syspll3_d4,
	syspll4_d4,
	syspll1_d16
};

static const char *const aud_intbus_parents[] __initconst = {
	clk26m,
	syspll1_d4,
	syspll4_d2
};

static const char *const pmicspi_parents[] __initconst = {
	clk26m,
	syspll1_d8,
	osc_d8
};

static const char *const atb_parents[] __initconst = {
	clk26m,
	syspll1_d2,
	syspll_d5
};

static const char *const dpi0_parents[] __initconst = {
	clk26m,
	tvdpll_d2,
	tvdpll_d4,
	tvdpll_d8,
	tvdpll_d16
};

static const char *const scam_parents[] __initconst = {
	clk26m,
	syspll3_d2
};

static const char *const aud_1_parents[] __initconst = {
	clk26m,
	apll1_ck
};

static const char *const aud_2_parents[] __initconst = {
	clk26m,
	apll2_ck
};

static const char *const disp_pwm_parents[] __initconst = {
	clk26m,
	univpll2_d4,
	osc_d4,
	osc_d8
};

static const char *const ssusb_top_sys_parents[] __initconst = {
	clk26m,
	univpll3_d2
};

static const char *const ssusb_top_xhci_parents[] __initconst = {
	clk26m,
	univpll3_d2
};

static const char *const usb_top_parents[] __initconst = {
	clk26m,
	univpll3_d4
};

static const char *const spm_parents[] __initconst = {
	clk26m,
	syspll1_d8
};

static const char *const bsi_spi_parents[] __initconst = {
	clk26m,
	syspll_d3_d3,
	syspll1_d4,
	syspll_d7
};

static const char *const i2c_parents[] __initconst = {
	clk26m,
	syspll1_d8,
	univpll3_d4
};

static const char *const dvfsp_parents[] __initconst = {
	clk26m,
	syspll1_d8
};

static const char *const venc_parents[] __initconst = {
	mm_sel,
};

static const char *const img_parents[] __initconst = {
	mm_sel,
};
/*
 * #define MUX_CLR_SET_UPD(_id, _name, _parents, _mux_ofs, _mux_set_ofs, \
 * _mux_clr_ofs, _shift, _width, _gate, _upd_ofs, _upd) {		\
 *		.id = _id,						\
 *		.name = _name,						\
 *		.mux_ofs = _mux_ofs,					\
 *		.mux_set_ofs = _mux_set_ofs,				\
 *		.mux_clr_ofs = _mux_clr_ofs,				\
 *		.upd_ofs = _upd_ofs,					\
 *		.mux_shift = _shift,					\
 *		.mux_width = _width,					\
 *		.gate_shift = _gate,					\
 *		.upd_shift = _upd,					\
 *		.parent_names = _parents,				\
 *		.num_parents = ARRAY_SIZE(_parents),			\
 *	}
 */

#define INVALID_UPDATE_REG 0xFFFFFFFF
#define INVALID_UPDATE_SHIFT -1
#define INVALID_MUX_GATE -1

static const struct mtk_mux_clr_set_upd top_muxes[] __initconst = {
/* CLK_CFG_0 */
	MUX_CLR_SET_UPD(TOP_MUX_AXI, axi_sel, axi_parents, CLK_CFG_0,
			CLK_CFG_0_SET, CLK_CFG_0_CLR, 0, 2, INVALID_MUX_GATE, CLK_CFG_UPDATE, 0),
/*MUX_CLR_SET_UPD(TOP_MUX_MEM, mem_sel, mem_parents, CLK_CFG_0,
 *     CLK_CFG_0_SET, CLK_CFG_0_CLR, 8, 2, 15, CLK_CFG_UPDATE, 1),
 *     MUX_CLR_SET_UPD(TOP_MUX_DDYPHY, ddrphy_sel, ddrphy_parents, CLK_CFG_0,
 *     CLK_CFG_0_SET, CLK_CFG_0_CLR, 16, 1, 23, CLK_CFG_UPDATE, 2),
 */
	MUX_CLR_SET_UPD(TOP_MUX_MM, mm_sel, mm_parents, CLK_CFG_0,
			CLK_CFG_0_SET, CLK_CFG_0_CLR, 24, 3, 31, CLK_CFG_UPDATE, 3),
/* CLK_CFG_1 */
	/*PWM no use */
	/*MUX_CLR_SET_UPD(TOP_MUX_PWM, pwm_sel, pwm_parents, CLK_CFG_1,
	 *  CLK_CFG_1_SET, CLK_CFG_1_CLR, 0, 2, 7, CLK_CFG_UPDATE, 4),
	 */
	MUX_CLR_SET_UPD(TOP_MUX_VDEC, vdec_sel, vdec_parents, CLK_CFG_1,
			CLK_CFG_1_SET, CLK_CFG_1_CLR, 8, 3, 15, CLK_CFG_UPDATE, 5),
	MUX_CLR_SET_UPD(TOP_MUX_VENC, venc_sel, venc_parents, CLK_CFG_1,
			CLK_CFG_1_SET, CLK_CFG_1_CLR, 16, 4, 23, CLK_CFG_UPDATE, 6),
	MUX_CLR_SET_UPD(TOP_MUX_MFG, mfg_sel, mfg_parents, CLK_CFG_1,
			CLK_CFG_1_SET, CLK_CFG_1_CLR, 24, 2, 31, CLK_CFG_UPDATE, 7),
/* CLK_CFG_2 */
	MUX_CLR_SET_UPD(TOP_MUX_CAMTG, camtg_sel, camtg_parents, CLK_CFG_2,
			CLK_CFG_2_SET, CLK_CFG_2_CLR, 0, 2, 7, CLK_CFG_UPDATE, 8),
	MUX_CLR_SET_UPD(TOP_MUX_UART, uart_sel, uart_parents, CLK_CFG_2,
			CLK_CFG_2_SET, CLK_CFG_2_CLR, 8, 1, 15, CLK_CFG_UPDATE, 9),
	MUX_CLR_SET_UPD(TOP_MUX_SPI, spi_sel, spi_parents, CLK_CFG_2,
			CLK_CFG_2_SET, CLK_CFG_2_CLR, 16, 2, 23, CLK_CFG_UPDATE, 10),
	/* no us20 */
	/*  MUX_CLR_SET_UPD(TOP_MUX_USB20, usb20_sel, usb20_parents, CLK_CFG_2,
	 *  CLK_CFG_2_SET, CLK_CFG_2_CLR, 24, 2, 31, CLK_CFG_UPDATE, ),
	 */
/* CLK_CFG_3 */
	MUX_CLR_SET_UPD(TOP_MUX_IMG, img_sel, img_parents, CLK_CFG_3,
			CLK_CFG_3_SET, CLK_CFG_3_CLR, 0, 1, 7, CLK_CFG_UPDATE,
			INVALID_UPDATE_SHIFT),
	MUX_CLR_SET_UPD(TOP_MUX_MSDC50_0_HCLK, msdc50_0_hclk_sel, msdc50_0_hclk_parents, CLK_CFG_3,
			CLK_CFG_3_SET, CLK_CFG_3_CLR, 8, 2, INVALID_MUX_GATE, CLK_CFG_UPDATE, 12),
	MUX_CLR_SET_UPD(TOP_MUX_MSDC50_0, msdc50_0_sel, msdc50_0_parents, CLK_CFG_3,
			CLK_CFG_3_SET, CLK_CFG_3_CLR, 16, 4, 23, CLK_CFG_UPDATE, 13),
	MUX_CLR_SET_UPD(TOP_MUX_MSDC30_1, msdc30_1_sel, msdc30_1_parents, CLK_CFG_3,
			CLK_CFG_3_SET, CLK_CFG_3_CLR, 24, 3, 31, CLK_CFG_UPDATE, 14),
/* CLK_CFG_4 */
	MUX_CLR_SET_UPD(TOP_MUX_MSDC30_2, msdc30_2_sel, msdc30_2_parents, CLK_CFG_4,
			CLK_CFG_4_SET, CLK_CFG_4_CLR, 0, 3, 7, CLK_CFG_UPDATE, 15),
	MUX_CLR_SET_UPD(TOP_MUX_MSDC30_3, msdc30_3_sel, msdc30_3_parents, CLK_CFG_4,
			CLK_CFG_4_SET, CLK_CFG_4_CLR, 8, 4, 15, CLK_CFG_UPDATE, 16),
	MUX_CLR_SET_UPD(TOP_MUX_AUDIO, audio_sel, audio_parents, CLK_CFG_4,
			CLK_CFG_4_SET, CLK_CFG_4_CLR, 16, 2, 23, CLK_CFG_UPDATE, 17),
	MUX_CLR_SET_UPD(TOP_MUX_AUD_INTBUS, aud_intbus_sel, aud_intbus_parents, CLK_CFG_4,
			CLK_CFG_4_SET, CLK_CFG_4_CLR, 24, 2, 31, CLK_CFG_UPDATE, 18),
/* CLK_CFG_5 */
	MUX_CLR_SET_UPD(TOP_MUX_PMICSPI, pmicspi_sel, pmicspi_parents, CLK_CFG_5,
			CLK_CFG_5_SET, CLK_CFG_5_CLR, 0, 2, INVALID_MUX_GATE, CLK_CFG_UPDATE, 19),
	/* no SCP */
	/* MUX_CLR_SET_UPD(TOP_MUX_SCP, scp_sel, pmicspi_parents, CLK_CFG_5,
	 * CLK_CFG_5_SET, CLK_CFG_5_CLR, 8, 2, 15, CLK_CFG_UPDATE, 20),
	 */
	MUX_CLR_SET_UPD(TOP_MUX_ATB, atb_sel, atb_parents, CLK_CFG_5,
			CLK_CFG_5_SET, CLK_CFG_5_CLR, 16, 2, INVALID_MUX_GATE, CLK_CFG_UPDATE, 21),
	/* MUX_CLR_SET_UPD(TOP_MUX_MJC, mjc_sel, mjc_parents, CLK_CFG_5,
	 * CLK_CFG_5_SET, CLK_CFG_5_CLR, 24, 2, 31, CLK_CFG_UPDATE, 22),
	 */
/* CLK_CFG_6 */
	MUX_CLR_SET_UPD(TOP_MUX_DPI0, dpi0_sel, dpi0_parents, CLK_CFG_6,
			CLK_CFG_6_SET, CLK_CFG_6_CLR, 0, 3, 7, CLK_CFG_UPDATE, 23),
	MUX_CLR_SET_UPD(TOP_MUX_SCAM, scam_sel, scam_parents, CLK_CFG_6,
			CLK_CFG_6_SET, CLK_CFG_6_CLR, 8, 1, 15, CLK_CFG_UPDATE, 24),
	MUX_CLR_SET_UPD(TOP_MUX_AUD_1, aud_1_sel, aud_1_parents, CLK_CFG_6,
			CLK_CFG_6_SET, CLK_CFG_6_CLR, 16, 1, 23, CLK_CFG_UPDATE, 25),
	MUX_CLR_SET_UPD(TOP_MUX_AUD_2, aud_2_sel, aud_2_parents, CLK_CFG_6,
			CLK_CFG_6_SET, CLK_CFG_6_CLR, 24, 1, 31, CLK_CFG_UPDATE, 26),
/* CLK_CFG_7 */
	MUX_CLR_SET_UPD(TOP_MUX_DISP_PWM, disp_pwm_sel, disp_pwm_parents, CLK_CFG_7,
			CLK_CFG_7_SET, CLK_CFG_7_CLR, 0, 2, 7, CLK_CFG_UPDATE, 27),
	MUX_CLR_SET_UPD(TOP_MUX_SSUSB_TOP_SYS, ssusb_top_sys_sel, ssusb_top_sys_parents, CLK_CFG_7,
			CLK_CFG_7_SET, CLK_CFG_7_CLR, 8, 1, 15, CLK_CFG_UPDATE, 28),
	MUX_CLR_SET_UPD(TOP_MUX_SSUSB_TOP_XHCI, ssusb_top_xhci_sel, ssusb_top_xhci_parents,
			CLK_CFG_7,
			CLK_CFG_7_SET, CLK_CFG_7_CLR, 16, 1, 23, CLK_CFG_UPDATE, 29),
	MUX_CLR_SET_UPD(TOP_MUX_USB_TOP, usb_top_sel, usb_top_parents, CLK_CFG_7,
			CLK_CFG_7_SET, CLK_CFG_7_CLR, 24, 1, 31, CLK_CFG_UPDATE, 30),
/* CLK_CFG_8 */
	MUX_CLR_SET_UPD(TOP_MUX_SPM, spm_sel, spm_parents, CLK_CFG_8,
			CLK_CFG_8_SET, CLK_CFG_8_CLR, 0, 1, INVALID_MUX_GATE, CLK_CFG_UPDATE, 31),
	MUX_CLR_SET_UPD(TOP_MUX_BSI_SPI, bsi_spi_sel, bsi_spi_parents, CLK_CFG_8,
			CLK_CFG_8_SET, CLK_CFG_8_CLR, 8, 2, INVALID_MUX_GATE, CLK_CFG_UPDATE1, 0),
	MUX_CLR_SET_UPD(TOP_MUX_I2C, i2c_sel, i2c_parents, CLK_CFG_8,
			CLK_CFG_8_SET, CLK_CFG_8_CLR, 16, 2, 23, CLK_CFG_UPDATE1, 1),
	MUX_CLR_SET_UPD(TOP_MUX_DVFSP, dvfsp_sel, dvfsp_parents, CLK_CFG_8,
			CLK_CFG_8_SET, CLK_CFG_8_CLR, 24, 1, 31, CLK_CFG_UPDATE1, 2),
/* CLK_CFG_9 */
	MUX_CLR_SET_UPD(TOP_MUX_F52M_MFG, f52m_mfg_sel, f52m_mfg_parents, CLK_CFG_9,
			CLK_CFG_9_SET, CLK_CFG_9_CLR, 0, 2, 7, CLK_CFG_UPDATE, 11),
};

/* CAMSYS */
#define camsys_larb2_cgpdn	"camsys_larb2_cgpdn"
#define camsys_camsys_cgpdn	"camsys_camsys_cgpdn"
#define camsys_camtg_cgpdn	"camsys_camtg_cgpdn"
#define camsys_seninf_cgpdn	"camsys_seninf_cgpdn"
#define camsys_camsv0_cgpdn	"camsys_camsv0_cgpdn"
#define camsys_camsv1_cgpdn	"camsys_camsv1_cgpdn"
#define camsys_camsv2_cgpdn	"camsys_camsv2_cgpdn"
#define camsys_tsf_cgpdn	"camsys_tsf_cgpdn"
/* INFRACFG_AO */
#define infracfg_ao_pmic_cg_tmr	"infracfg_ao_pmic_cg_tmr"
#define infracfg_ao_pmic_cg_ap	"infracfg_ao_pmic_cg_ap"
#define infracfg_ao_pmic_cg_md	"infracfg_ao_pmic_cg_md"
#define infracfg_ao_pmic_cg_conn	"infracfg_ao_pmic_cg_conn"
#define infracfg_ao_sej_cg	"infracfg_ao_sej_cg"
#define infracfg_ao_apxgpt_cg	"infracfg_ao_apxgpt_cg"
#define infracfg_ao_icusb_cg	"infracfg_ao_icusb_cg"
#define infracfg_ao_gce_cg	"infracfg_ao_gce_cg"
#define infracfg_ao_therm_cg	"infracfg_ao_therm_cg"
#define infracfg_ao_i2c0_cg	"infracfg_ao_i2c0_cg"
#define infracfg_ao_i2c1_cg	"infracfg_ao_i2c1_cg"
#define infracfg_ao_i2c2_cg	"infracfg_ao_i2c2_cg"
#define infracfg_ao_i2c3_cg	"infracfg_ao_i2c3_cg"
#define infracfg_ao_pwm_hclk_cg	"infracfg_ao_pwm_hclk_cg"
#define infracfg_ao_pwm1_cg	"infracfg_ao_pwm1_cg"
#define infracfg_ao_pwm2_cg	"infracfg_ao_pwm2_cg"
#define infracfg_ao_pwm3_cg	"infracfg_ao_pwm3_cg"
#define infracfg_ao_pwm4_cg	"infracfg_ao_pwm4_cg"
#define infracfg_ao_pwm_cg	"infracfg_ao_pwm_cg"
#define infracfg_ao_uart0_cg	"infracfg_ao_uart0_cg"
#define infracfg_ao_uart1_cg	"infracfg_ao_uart1_cg"
#define infracfg_ao_uart2_cg	"infracfg_ao_uart2_cg"
#define infracfg_ao_uart3_cg	"infracfg_ao_uart3_cg"
#define infracfg_ao_md2md_ccif_cg0	"infracfg_ao_md2md_ccif_cg0"
#define infracfg_ao_md2md_ccif_cg1	"infracfg_ao_md2md_ccif_cg1"
#define infracfg_ao_md2md_ccif_cg2	"infracfg_ao_md2md_ccif_cg2"
#define infracfg_ao_btif_cg	"infracfg_ao_btif_cg"
#define infracfg_ao_md2md_ccif_cg3	"infracfg_ao_md2md_ccif_cg3"
#define infracfg_ao_spi0_cg	"infracfg_ao_spi0_cg"
#define infracfg_ao_msdc0_cg	"infracfg_ao_msdc0_cg"
#define infracfg_ao_md2md_ccif_cg4	"infracfg_ao_md2md_ccif_cg4"
#define infracfg_ao_msdc1_cg	"infracfg_ao_msdc1_cg"
#define infracfg_ao_msdc2_cg	"infracfg_ao_msdc2_cg"
#define infracfg_ao_msdc3_cg	"infracfg_ao_msdc3_cg"
#define infracfg_ao_md2md_ccif_cg5	"infracfg_ao_md2md_ccif_cg5"
#define infracfg_ao_gcpu_cg	"infracfg_ao_gcpu_cg"
#define infracfg_ao_trng_cg	"infracfg_ao_trng_cg"
#define infracfg_ao_auxadc_cg	"infracfg_ao_auxadc_cg"
#define infracfg_ao_cpum_cg	"infracfg_ao_cpum_cg"
#define infracfg_ao_ccif1_ap_cg	"infracfg_ao_ccif1_ap_cg"
#define infracfg_ao_ccif1_md_cg	"infracfg_ao_ccif1_md_cg"
#define infracfg_ao_nfi_cg	"infracfg_ao_nfi_cg"
#define infracfg_ao_nfi_ecc_cg	"infracfg_ao_nfi_ecc_cg"
#define infracfg_ao_ap_dma_cg	"infracfg_ao_ap_dma_cg"
#define infracfg_ao_xiu_cg	"infracfg_ao_xiu_cg"
#define infracfg_ao_device_apc_cg	"infracfg_ao_device_apc_cg"
#define infracfg_ao_xiu2ahb_cg	"infracfg_ao_xiu2ahb_cg"
#define infracfg_ao_l2c_sram_cg	"infracfg_ao_l2c_sram_cg"
#define infracfg_ao_ccif_ap_cg	"infracfg_ao_ccif_ap_cg"
#define infracfg_ao_debugsys_cg	"infracfg_ao_debugsys_cg"
#define infracfg_ao_audio_cg	"infracfg_ao_audio_cg"
#define infracfg_ao_ccif_md_cg	"infracfg_ao_ccif_md_cg"
#define infracfg_ao_dramc_f26m_cg	"infracfg_ao_dramc_f26m_cg"
#define infracfg_ao_irtx_cg	"infracfg_ao_irtx_cg"
#define infracfg_ao_ssusb_top_cg	"infracfg_ao_ssusb_top_cg"
#define infracfg_ao_disp_pwm_cg	"infracfg_ao_disp_pwm_cg"
#define infracfg_ao_cldma_bclk_ck	"infracfg_ao_cldma_bclk_ck"
#define infracfg_ao_audio_26m_bclk_ck	"infracfg_ao_audio_26m_bclk_ck"
#define infracfg_ao_modem_temp_26m_bclk_ck	"infracfg_ao_modem_temp_26m_bclk_ck"
#define infracfg_ao_spi1_cg	"infracfg_ao_spi1_cg"
#define infracfg_ao_i2c4_cg	"infracfg_ao_i2c4_cg"
#define infracfg_ao_modem_temp_share_cg	"infracfg_ao_modem_temp_share_cg"
#define infracfg_ao_spi2_cg	"infracfg_ao_spi2_cg"
#define infracfg_ao_spi3_cg	"infracfg_ao_spi3_cg"
#define infracfg_ao_i2c5_cg	"infracfg_ao_i2c5_cg"
#define infracfg_ao_i2c5_arbiter_cg	"infracfg_ao_i2c5_arbiter_cg"
#define infracfg_ao_i2c5_imm_cg	"infracfg_ao_i2c5_imm_cg"
#define infracfg_ao_i2c1_arbiter_cg	"infracfg_ao_i2c1_arbiter_cg"
#define infracfg_ao_i2c1_imm_cg	"infracfg_ao_i2c1_imm_cg"
#define infracfg_ao_i2c2_arbiter_cg	"infracfg_ao_i2c2_arbiter_cg"
#define infracfg_ao_i2c2_imm_cg	"infracfg_ao_i2c2_imm_cg"
#define infracfg_ao_spi4_cg	"infracfg_ao_spi4_cg"
#define infracfg_ao_spi5_cg	"infracfg_ao_spi5_cg"
#define infracfg_ao_peri_dcm_rg_force_clkoff	"infracfg_ao_peri_dcm_rg_force_clkoff"
/* MFGCFG */
#define mfgcfg_bg3d	"mfgcfg_bg3d"
/* IMG */
#define	img_rsc "img_rsc"
#define	img_gepf "img_gepf"
#define	img_fdvt "img_fdvt"
#define	img_dpe "img_dpe"
#define	img_dip "img_dip"
#define	img_dfp_vad "img_dfp_vad"
#define	img_larb5 "img_larb5"
/* MMSYS_CONFIG */
#define	mmsys_smi_common           "mmsys_smi_common"
#define	mmsys_smi_larb0            "mmsys_smi_larb0"
#define	mmsys_smi_larb4            "mmsys_smi_larb4"
#define	mmsys_cam_mdp              "mmsys_cam_mdp"
#define	mmsys_mdp_rdma0            "mmsys_mdp_rdma0"
#define	mmsys_mdp_rdma1            "mmsys_mdp_rdma1"
#define	mmsys_mdp_rsz0             "mmsys_mdp_rsz0"
#define	mmsys_mdp_rsz1             "mmsys_mdp_rsz1"
#define	mmsys_mdp_rsz2             "mmsys_mdp_rsz2"
#define	mmsys_mdp_tdshp            "mmsys_mdp_tdshp"
#define	mmsys_mdp_color            "mmsys_mdp_color"
#define	mmsys_mdp_wdma             "mmsys_mdp_wdma"
#define	mmsys_mdp_wrot0            "mmsys_mdp_wrot0"
#define	mmsys_mdp_wrot1            "mmsys_mdp_wrot1"
#define	mmsys_disp_ovl0            "mmsys_disp_ovl0"
#define	mmsys_disp_ovl1            "mmsys_disp_ovl1"
#define	mmsys_disp_ovl0_2l         "mmsys_disp_ovl0_2l"
#define	mmsys_disp_ovl1_2l         "mmsys_disp_ovl1_2l"
#define	mmsys_disp_rdma0           "mmsys_disp_rdma0"
#define	mmsys_disp_rdma1           "mmsys_disp_rdma1"
#define	mmsys_disp_rdma2           "mmsys_disp_rdma2"
#define	mmsys_disp_wdma0           "mmsys_disp_wdma0"
#define	mmsys_disp_wdma1           "mmsys_disp_wdma1"
#define	mmsys_disp_color0          "mmsys_disp_color0"
#define	mmsys_disp_color1          "mmsys_disp_color1"
#define	mmsys_disp_ccorr0          "mmsys_disp_ccorr0"
#define	mmsys_disp_ccorr1          "mmsys_disp_ccorr1"
#define	mmsys_disp_aal0            "mmsys_disp_aal0"
#define	mmsys_disp_aal1            "mmsys_disp_aal1"
#define	mmsys_disp_gamma0          "mmsys_disp_gamma0"
#define	mmsys_disp_gamma1          "mmsys_disp_gamma1"
#define	mmsys_disp_od              "mmsys_disp_od"
#define	mmsys_disp_dither0         "mmsys_disp_dither0"
#define	mmsys_disp_dither1         "mmsys_disp_dither1"
#define	mmsys_disp_ufoe            "mmsys_disp_ufoe"
#define	mmsys_disp_dsc             "mmsys_disp_dsc"
#define	mmsys_disp_split           "mmsys_disp_split"
#define	mmsys_dsi0_mm_clock        "mmsys_dsi0_mm_clock"
#define	mmsys_dsi0_interface_clock "mmsys_dsi0_interface_clock"
#define	mmsys_dsi1_mm_clock        "mmsys_dsi1_mm_clock"
#define	mmsys_dsi1_interface_clock "mmsys_dsi1_interface_clock"
#define	mmsys_dpi_mm_clock         "mmsys_dpi_mm_clock"
#define	mmsys_dpi_interface_clock  "mmsys_dpi_interface_clock"
#define	mmsys_disp_ovl0_mout_clock "mmsys_disp_ovl0_mout_clock"

/* VDEC_GCON */
#define	vdec_vdec "vdec_vdec"
#define	vdec_larb1 "vdec_larb1"

/* VENC_GCON */
#define venc_gcon_set0_larb	"venc_gcon_set0_larb"
#define venc_gcon_set1_venc	"venc_gcon_set1_venc"
#define venc_gcon_set2_jpgenc	"venc_gcon_set2_jpgenc"
#define venc_gcon_set3_jpgdec	"venc_gcon_set3_jpgdec"

/* AUDIO */
#define	audio_tml	"audio_tml"
#define	audio_dac_predis	"audio_dac_predis"
#define	audio_dac	"audio_dac"
#define	audio_adc	"audio_adc"
#define	audio_apll_tuner	"audio_apll_tuner"
#define	audio_apll2_tuner	"audio_apll2_tuner"
#define	audio_24m	"audio_24m"
#define	audio_22m	"audio_22m"
#define	audio_i2s	"audio_i2s"
#define	audio_afe	"audio_afe"

static const struct mtk_gate_regs camsys_camsys_cg_regs = {
	.set_ofs = 0x4,
	.clr_ofs = 0x8,
	.sta_ofs = 0x0,
};

static const struct mtk_gate camsys_clks[] __initconst = {
	/* CAMSYS_CG */
	GATE(CAMSYS_LARB2_CGPDN, camsys_larb2_cgpdn, img_sel,
	     camsys_camsys_cg_regs, 0, 0),
	GATE(CAMSYS_CAMSYS_CGPDN, camsys_camsys_cgpdn, img_sel,
	     camsys_camsys_cg_regs, 6, 0),
	GATE(CAMSYS_CAMTG_CGPDN, camsys_camtg_cgpdn, camtg_sel,
	     camsys_camsys_cg_regs, 7, 0),
	GATE(CAMSYS_SENINF_CGPDN, camsys_seninf_cgpdn, img_sel,
	     camsys_camsys_cg_regs, 8, 0),
	GATE(CAMSYS_CAMSV0_CGPDN, camsys_camsv0_cgpdn, img_sel,
	     camsys_camsys_cg_regs, 9, 0),
	GATE(CAMSYS_CAMSV1_CGPDN, camsys_camsv1_cgpdn, img_sel,
	     camsys_camsys_cg_regs, 10, 0),
	GATE(CAMSYS_CAMSV2_CGPDN, camsys_camsv2_cgpdn, img_sel,
	     camsys_camsys_cg_regs, 11, 0),
	GATE(CAMSYS_TSF_CGPDN, camsys_tsf_cgpdn, img_sel,
	     camsys_camsys_cg_regs, 12, 0),
};

static const struct mtk_gate_regs infracfg_ao_module_sw_cg_0_regs = {
	.set_ofs = 0x80,
	.clr_ofs = 0x84,
	.sta_ofs = 0x90,
};

static const struct mtk_gate_regs infracfg_ao_module_sw_cg_1_regs = {
	.set_ofs = 0x88,
	.clr_ofs = 0x8c,
	.sta_ofs = 0x94,
};

static const struct mtk_gate_regs infracfg_ao_module_sw_cg_2_regs = {
	.set_ofs = 0xa4,
	.clr_ofs = 0xa8,
	.sta_ofs = 0xac,
};

static const struct mtk_gate infracfg_ao_clks[] __initconst = {
	/* MODULE_SW_CG_0 */
	GATE(INFRACFG_AO_PMIC_CG_TMR, infracfg_ao_pmic_cg_tmr, pmicspi_sel,
	     infracfg_ao_module_sw_cg_0_regs, 0, 0),
	GATE(INFRACFG_AO_PMIC_CG_AP, infracfg_ao_pmic_cg_ap, pmicspi_sel,
	     infracfg_ao_module_sw_cg_0_regs, 1, 0),
	GATE(INFRACFG_AO_PMIC_CG_MD, infracfg_ao_pmic_cg_md, pmicspi_sel,
	     infracfg_ao_module_sw_cg_0_regs, 2, 0),
	GATE(INFRACFG_AO_PMIC_CG_CONN, infracfg_ao_pmic_cg_conn, pmicspi_sel,
	     infracfg_ao_module_sw_cg_0_regs, 3, 0),
	GATE(INFRACFG_AO_SEJ_CG, infracfg_ao_sej_cg, f26m_sel,
	     infracfg_ao_module_sw_cg_0_regs, 5, 0),
	GATE(INFRACFG_AO_APXGPT_CG, infracfg_ao_apxgpt_cg, axi_sel,
	     infracfg_ao_module_sw_cg_0_regs, 6, 0),
	GATE(INFRACFG_AO_ICUSB_CG, infracfg_ao_icusb_cg, axi_sel,
	     infracfg_ao_module_sw_cg_0_regs, 8, 0),
	GATE(INFRACFG_AO_GCE_CG, infracfg_ao_gce_cg, axi_sel,
	     infracfg_ao_module_sw_cg_0_regs, 9, 0),
	GATE(INFRACFG_AO_THERM_CG, infracfg_ao_therm_cg, axi_sel,
	     infracfg_ao_module_sw_cg_0_regs, 10, 0),
	GATE(INFRACFG_AO_I2C0_CG, infracfg_ao_i2c0_cg, i2c_sel,
	     infracfg_ao_module_sw_cg_0_regs, 11, 0),
	GATE(INFRACFG_AO_I2C1_CG, infracfg_ao_i2c1_cg, i2c_sel,
	     infracfg_ao_module_sw_cg_0_regs, 12, 0),
	GATE(INFRACFG_AO_I2C2_CG, infracfg_ao_i2c2_cg, i2c_sel,
	     infracfg_ao_module_sw_cg_0_regs, 13, 0),
	GATE(INFRACFG_AO_I2C3_CG, infracfg_ao_i2c3_cg, i2c_sel,
	     infracfg_ao_module_sw_cg_0_regs, 14, 0),
	GATE(INFRACFG_AO_PWM_HCLK_CG, infracfg_ao_pwm_hclk_cg, axi_sel,
	     infracfg_ao_module_sw_cg_0_regs, 15, 0),
	GATE(INFRACFG_AO_PWM1_CG, infracfg_ao_pwm1_cg, i2c_sel,
	     infracfg_ao_module_sw_cg_0_regs, 16, 0),
	GATE(INFRACFG_AO_PWM2_CG, infracfg_ao_pwm2_cg, i2c_sel,
	     infracfg_ao_module_sw_cg_0_regs, 17, 0),
	GATE(INFRACFG_AO_PWM3_CG, infracfg_ao_pwm3_cg, i2c_sel,
	     infracfg_ao_module_sw_cg_0_regs, 18, 0),
	GATE(INFRACFG_AO_PWM4_CG, infracfg_ao_pwm4_cg, i2c_sel,
	     infracfg_ao_module_sw_cg_0_regs, 19, 0),
	GATE(INFRACFG_AO_PWM_CG, infracfg_ao_pwm_cg, i2c_sel,
	     infracfg_ao_module_sw_cg_0_regs, 21, 0),
	GATE(INFRACFG_AO_UART0_CG, infracfg_ao_uart0_cg, uart_sel,
	     infracfg_ao_module_sw_cg_0_regs, 22, 0),
	GATE(INFRACFG_AO_UART1_CG, infracfg_ao_uart1_cg, uart_sel,
	     infracfg_ao_module_sw_cg_0_regs, 23, 0),
	GATE(INFRACFG_AO_UART2_CG, infracfg_ao_uart2_cg, uart_sel,
	     infracfg_ao_module_sw_cg_0_regs, 24, 0),
	GATE(INFRACFG_AO_UART3_CG, infracfg_ao_uart3_cg, uart_sel,
	     infracfg_ao_module_sw_cg_0_regs, 25, 0),
	GATE(INFRACFG_AO_MD2MD_CCIF_CG0, infracfg_ao_md2md_ccif_cg0, axi_sel,
	     infracfg_ao_module_sw_cg_0_regs, 27, 0),
	GATE(INFRACFG_AO_MD2MD_CCIF_CG1, infracfg_ao_md2md_ccif_cg1, axi_sel,
	     infracfg_ao_module_sw_cg_0_regs, 28, 0),
	GATE(INFRACFG_AO_MD2MD_CCIF_CG2, infracfg_ao_md2md_ccif_cg2, axi_sel,
	     infracfg_ao_module_sw_cg_0_regs, 29, 0),
	GATE(INFRACFG_AO_BTIF_CG, infracfg_ao_btif_cg, axi_sel,
	     infracfg_ao_module_sw_cg_0_regs, 31, 0),
	/* MODULE_SW_CG_1 */
	GATE(INFRACFG_AO_MD2MD_CCIF_CG3, infracfg_ao_md2md_ccif_cg3, axi_sel,
	     infracfg_ao_module_sw_cg_1_regs, 0, 0),
	GATE(INFRACFG_AO_SPI0_CG, infracfg_ao_spi0_cg, spi_sel,
	     infracfg_ao_module_sw_cg_1_regs, 1, 0),
	GATE(INFRACFG_AO_MSDC0_CG, infracfg_ao_msdc0_cg, msdc50_0_sel,
	     infracfg_ao_module_sw_cg_1_regs, 2, 0),
	GATE(INFRACFG_AO_MD2MD_CCIF_CG4, infracfg_ao_md2md_ccif_cg4, axi_sel,
	     infracfg_ao_module_sw_cg_1_regs, 3, 0),
	GATE(INFRACFG_AO_MSDC1_CG, infracfg_ao_msdc1_cg, msdc30_1_sel,
	     infracfg_ao_module_sw_cg_1_regs, 4, 0),
	GATE(INFRACFG_AO_MSDC2_CG, infracfg_ao_msdc2_cg, msdc30_2_sel,
	     infracfg_ao_module_sw_cg_1_regs, 5, 0),
	GATE(INFRACFG_AO_MSDC3_CG, infracfg_ao_msdc3_cg, msdc30_3_sel,
	     infracfg_ao_module_sw_cg_1_regs, 6, 0),
	GATE(INFRACFG_AO_MD2MD_CCIF_CG5, infracfg_ao_md2md_ccif_cg5, axi_sel,
	     infracfg_ao_module_sw_cg_1_regs, 7, 0),
	GATE(INFRACFG_AO_GCPU_CG, infracfg_ao_gcpu_cg, axi_sel,
	     infracfg_ao_module_sw_cg_1_regs, 8, 0),
	GATE(INFRACFG_AO_TRNG_CG, infracfg_ao_trng_cg, axi_sel,
	     infracfg_ao_module_sw_cg_1_regs, 9, 0),
	GATE(INFRACFG_AO_AUXADC_CG, infracfg_ao_auxadc_cg, f26m_sel,
	     infracfg_ao_module_sw_cg_1_regs, 10, 0),
	GATE(INFRACFG_AO_CPUM_CG, infracfg_ao_cpum_cg, axi_sel,
	     infracfg_ao_module_sw_cg_1_regs, 11, 0),
	GATE(INFRACFG_AO_CCIF1_AP_CG, infracfg_ao_ccif1_ap_cg, axi_sel,
	     infracfg_ao_module_sw_cg_1_regs, 12, 0),
	GATE(INFRACFG_AO_CCIF1_MD_CG, infracfg_ao_ccif1_md_cg, axi_sel,
	     infracfg_ao_module_sw_cg_1_regs, 13, 0),
	/*no NFI */
	GATE(INFRACFG_AO_AP_DMA_CG, infracfg_ao_ap_dma_cg, axi_sel,
	     infracfg_ao_module_sw_cg_1_regs, 18, 0),
	GATE(INFRACFG_AO_XIU_CG, infracfg_ao_xiu_cg, axi_sel,
	     infracfg_ao_module_sw_cg_1_regs, 19, 0),
	GATE(INFRACFG_AO_DEVICE_APC_CG, infracfg_ao_device_apc_cg, axi_sel,
	     infracfg_ao_module_sw_cg_1_regs, 20, 0),
	GATE(INFRACFG_AO_XIU2AHB_CG, infracfg_ao_xiu2ahb_cg, axi_sel,
	     infracfg_ao_module_sw_cg_1_regs, 21, 0),
	GATE(INFRACFG_AO_CCIF_AP_CG, infracfg_ao_ccif_ap_cg, axi_sel,
	     infracfg_ao_module_sw_cg_1_regs, 23, 0),
	GATE(INFRACFG_AO_DEBUGSYS_CG, infracfg_ao_debugsys_cg, axi_sel,
	     infracfg_ao_module_sw_cg_1_regs, 24, 0),
	GATE(INFRACFG_AO_AUDIO_CG, infracfg_ao_audio_cg, axi_sel,
	     infracfg_ao_module_sw_cg_1_regs, 25, 0),
	GATE(INFRACFG_AO_CCIF_MD_CG, infracfg_ao_ccif_md_cg, axi_sel,
	     infracfg_ao_module_sw_cg_1_regs, 26, 0),
	GATE(INFRACFG_AO_DRAMC_F26M_CG, infracfg_ao_dramc_f26m_cg, f26m_sel,
	     infracfg_ao_module_sw_cg_1_regs, 31, 0),
	/* MODULE_SW_CG_2 */
	GATE(INFRACFG_AO_IRTX_CG, infracfg_ao_irtx_cg, f26m_sel,
	     infracfg_ao_module_sw_cg_2_regs, 0, 0),
	GATE(INFRACFG_AO_SSUSB_TOP_CG, infracfg_ao_ssusb_top_cg, ssusb_top_sys_sel,
	     infracfg_ao_module_sw_cg_2_regs, 1, 0),
	GATE(INFRACFG_AO_DISP_PWM_CG, infracfg_ao_disp_pwm_cg, disp_pwm_sel,
	     infracfg_ao_module_sw_cg_2_regs, 2, 0),
	GATE(INFRACFG_AO_CLDMA_BCLK_CK, infracfg_ao_cldma_bclk_ck, axi_sel,
	     infracfg_ao_module_sw_cg_2_regs, 3, 0),
	GATE(INFRACFG_AO_AUDIO_26M_BCLK_CK, infracfg_ao_audio_26m_bclk_ck, f26m_sel,
	     infracfg_ao_module_sw_cg_2_regs, 4, 0),
	GATE(INFRACFG_AO_SPI1_CG, infracfg_ao_spi1_cg, spi_sel,
	     infracfg_ao_module_sw_cg_2_regs, 6, 0),
	GATE(INFRACFG_AO_I2C4_CG, infracfg_ao_i2c4_cg, i2c_sel,
	     infracfg_ao_module_sw_cg_2_regs, 7, 0),
	GATE(INFRACFG_AO_MODEM_TEMP_SHARE_CG, infracfg_ao_modem_temp_share_cg, f26m_sel,
	     infracfg_ao_module_sw_cg_2_regs, 8, 0),
	GATE(INFRACFG_AO_SPI2_CG, infracfg_ao_spi2_cg, spi_sel,
	     infracfg_ao_module_sw_cg_2_regs, 9, 0),
	GATE(INFRACFG_AO_SPI3_CG, infracfg_ao_spi3_cg, spi_sel,
	     infracfg_ao_module_sw_cg_2_regs, 10, 0),
	GATE(INFRACFG_AO_I2C5_CG, infracfg_ao_i2c5_cg, i2c_sel,
	     infracfg_ao_module_sw_cg_2_regs, 18, 0),
	GATE(INFRACFG_AO_I2C5_ARBITER_CG, infracfg_ao_i2c5_arbiter_cg, i2c_sel,
	     infracfg_ao_module_sw_cg_2_regs, 19, 0),
	GATE(INFRACFG_AO_I2C5_IMM_CG, infracfg_ao_i2c5_imm_cg, i2c_sel,
	     infracfg_ao_module_sw_cg_2_regs, 20, 0),
	GATE(INFRACFG_AO_I2C1_ARBITER_CG, infracfg_ao_i2c1_arbiter_cg, i2c_sel,
	     infracfg_ao_module_sw_cg_2_regs, 21, 0),
	GATE(INFRACFG_AO_I2C1_IMM_CG, infracfg_ao_i2c1_imm_cg, i2c_sel,
	     infracfg_ao_module_sw_cg_2_regs, 22, 0),
	GATE(INFRACFG_AO_I2C2_ARBITER_CG, infracfg_ao_i2c2_arbiter_cg, i2c_sel,
	     infracfg_ao_module_sw_cg_2_regs, 23, 0),
	GATE(INFRACFG_AO_I2C2_IMM_CG, infracfg_ao_i2c2_imm_cg, i2c_sel,
	     infracfg_ao_module_sw_cg_2_regs, 24, 0),
	GATE(INFRACFG_AO_SPI4_CG, infracfg_ao_spi4_cg, spi_sel,
	     infracfg_ao_module_sw_cg_2_regs, 25, 0),
	GATE(INFRACFG_AO_SPI5_CG, infracfg_ao_spi5_cg, spi_sel,
	     infracfg_ao_module_sw_cg_2_regs, 26, 0),
};

static const struct mtk_gate_regs mfgcfg_mfg_cg_regs = {
	.set_ofs = 0x4,
	.clr_ofs = 0x8,
	.sta_ofs = 0x0,
};

static const struct mtk_gate mfgcfg_clks[] __initconst = {
	/* MFG_CG */
	GATE(MFGCFG_BG3D, mfgcfg_bg3d, mfg_sel,
	     mfgcfg_mfg_cg_regs, 0, 0),
};

static const struct mtk_gate_regs img_cg_regs = {
	.set_ofs = 0x4,
	.clr_ofs = 0x8,
	.sta_ofs = 0x0,
};

static const struct mtk_gate img_clks[] __initconst = {
	GATE(IMG_RSC, img_rsc, img_sel, img_cg_regs, 13, 0),
	GATE(IMG_GEPF, img_gepf, img_sel, img_cg_regs, 12, 0),
	GATE(IMG_FDVT, img_fdvt, img_sel, img_cg_regs, 11, 0),
	GATE(IMG_DPE, img_dpe, img_sel, img_cg_regs, 10, 0),
	GATE(IMG_DIP, img_dip, img_sel, img_cg_regs, 6, 0),
	GATE(IMG_DFP_VAD, img_dfp_vad, img_sel, img_cg_regs, 1, 0),
	GATE(IMG_LARB5, img_larb5, img_sel, img_cg_regs, 0, 0),
};

static const struct mtk_gate_regs mmsys_cg_con0_regs = {
	.set_ofs = 0x104,
	.clr_ofs = 0x108,
	.sta_ofs = 0x100,
};

static const struct mtk_gate_regs mmsys_cg_con1_regs = {
	.set_ofs = 0x114,
	.clr_ofs = 0x118,
	.sta_ofs = 0x110,
};

static const struct mtk_gate_regs mmsys_dummy_cg_con0_regs = {
	.set_ofs = 0x08a8,
	.clr_ofs = 0x08a8,
	.sta_ofs = 0x08a8,
};

static const struct mtk_gate_regs mmsys_dummy_cg_con1_regs = {
	.set_ofs = 0x08ac,
	.clr_ofs = 0x08ac,
	.sta_ofs = 0x08ac,
};


static const struct mtk_gate mmsys_config_clks[] __initconst = {
	GATE(MMSYS_SMI_COMMON, mmsys_smi_common, mm_sel, mmsys_cg_con0_regs, 0, 0),
	GATE(MMSYS_SMI_LARB0, mmsys_smi_larb0, mm_sel, mmsys_cg_con0_regs, 1, 0),
	GATE(MMSYS_SMI_LARB4, mmsys_smi_larb4, mm_sel, mmsys_cg_con0_regs, 2, 0),
	GATE(MMSYS_CAM_MDP, mmsys_cam_mdp, mm_sel, mmsys_dummy_cg_con0_regs, 3,
	     CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MMSYS_MDP_RDMA0, mmsys_mdp_rdma0, mm_sel, mmsys_dummy_cg_con0_regs, 4,
	     CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MMSYS_MDP_RDMA1, mmsys_mdp_rdma1, mm_sel, mmsys_dummy_cg_con0_regs, 5,
	     CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MMSYS_MDP_RSZ0, mmsys_mdp_rsz0, mm_sel, mmsys_dummy_cg_con0_regs, 6,
	     CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MMSYS_MDP_RSZ1, mmsys_mdp_rsz1, mm_sel, mmsys_dummy_cg_con0_regs, 7,
	     CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MMSYS_MDP_RSZ2, mmsys_mdp_rsz2, mm_sel, mmsys_dummy_cg_con0_regs, 8,
	     CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MMSYS_MDP_TDSHP, mmsys_mdp_tdshp, mm_sel, mmsys_dummy_cg_con0_regs, 9,
	     CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MMSYS_MDP_COLOR, mmsys_mdp_color, mm_sel, mmsys_dummy_cg_con0_regs, 10,
	     CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MMSYS_MDP_WDMA, mmsys_mdp_wdma, mm_sel, mmsys_dummy_cg_con0_regs, 11,
	     CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MMSYS_MDP_WROT0, mmsys_mdp_wrot0, mm_sel, mmsys_dummy_cg_con0_regs, 12,
	     CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MMSYS_MDP_WROT1, mmsys_mdp_wrot1, mm_sel, mmsys_dummy_cg_con0_regs, 13,
	     CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MMSYS_DISP_OVL0, mmsys_disp_ovl0, mm_sel, mmsys_dummy_cg_con0_regs, 15,
	     CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MMSYS_DISP_OVL1, mmsys_disp_ovl1, mm_sel, mmsys_dummy_cg_con0_regs, 16,
	     CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MMSYS_DISP_OVL0_2L, mmsys_disp_ovl0_2l, mm_sel, mmsys_dummy_cg_con0_regs, 17,
	     CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MMSYS_DISP_OVL1_2L, mmsys_disp_ovl1_2l, mm_sel, mmsys_dummy_cg_con0_regs, 18,
	     CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MMSYS_DISP_RDMA0, mmsys_disp_rdma0, mm_sel, mmsys_dummy_cg_con0_regs, 19,
	     CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MMSYS_DISP_RDMA1, mmsys_disp_rdma1, mm_sel, mmsys_dummy_cg_con0_regs, 20,
	     CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MMSYS_DISP_RDMA2, mmsys_disp_rdma2, mm_sel, mmsys_dummy_cg_con0_regs, 21,
	     CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MMSYS_DISP_WDMA0, mmsys_disp_wdma0, mm_sel, mmsys_dummy_cg_con0_regs, 22,
	     CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MMSYS_DISP_WDMA1, mmsys_disp_wdma1, mm_sel, mmsys_dummy_cg_con0_regs, 23,
	     CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MMSYS_DISP_COLOR0, mmsys_disp_color0, mm_sel, mmsys_dummy_cg_con0_regs, 24,
	     CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MMSYS_DISP_COLOR1, mmsys_disp_color1, mm_sel, mmsys_dummy_cg_con0_regs, 25,
	     CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MMSYS_DISP_CCORR0, mmsys_disp_ccorr0, mm_sel, mmsys_dummy_cg_con0_regs, 26,
	     CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MMSYS_DISP_CCORR1, mmsys_disp_ccorr1, mm_sel, mmsys_dummy_cg_con0_regs, 27,
	     CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MMSYS_DISP_AAL0, mmsys_disp_aal0, mm_sel, mmsys_dummy_cg_con0_regs, 28,
	     CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MMSYS_DISP_AAL1, mmsys_disp_aal1, mm_sel, mmsys_dummy_cg_con0_regs, 29,
	     CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MMSYS_DISP_GAMMA0, mmsys_disp_gamma0, mm_sel, mmsys_dummy_cg_con0_regs, 30,
	     CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MMSYS_DISP_GAMMA1, mmsys_disp_gamma1, mm_sel, mmsys_dummy_cg_con0_regs, 31,
	     CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MMSYS_DISP_OD, mmsys_disp_od, mm_sel, mmsys_dummy_cg_con1_regs, 0,
	     CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MMSYS_DISP_DITHER0, mmsys_disp_dither0, mm_sel, mmsys_dummy_cg_con1_regs, 1,
	     CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MMSYS_DISP_DITHER1, mmsys_disp_dither1, mm_sel, mmsys_dummy_cg_con1_regs, 2,
	     CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MMSYS_DISP_UFOE, mmsys_disp_ufoe, mm_sel, mmsys_dummy_cg_con1_regs, 3,
	     CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MMSYS_DISP_DSC, mmsys_disp_dsc, mm_sel, mmsys_dummy_cg_con1_regs, 4,
	     CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MMSYS_DISP_SPLIT, mmsys_disp_split, mm_sel, mmsys_dummy_cg_con1_regs, 5,
	     CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
	GATE(MMSYS_DSI0_MM_CLOCK, mmsys_dsi0_mm_clock, mm_sel, mmsys_cg_con1_regs, 6, 0),
	GATE(MMSYS_DSI0_INTERFACE_CLOCK, mmsys_dsi0_interface_clock, f26m_sel, mmsys_cg_con1_regs,
	     7, 0),
	GATE(MMSYS_DSI1_MM_CLOCK, mmsys_dsi1_mm_clock, mm_sel, mmsys_cg_con1_regs, 8, 0),
	GATE(MMSYS_DSI1_INTERFACE_CLOCK, mmsys_dsi1_interface_clock, f26m_sel, mmsys_cg_con1_regs,
	     9, 0),
	GATE(MMSYS_DPI_MM_CLOCK, mmsys_dpi_mm_clock, mm_sel, mmsys_cg_con1_regs, 10, 0),
	GATE(MMSYS_DPI_INTERFACE_CLOCK, mmsys_dpi_interface_clock, dpi0_sel, mmsys_cg_con1_regs, 11,
	     0),
	GATE(MMSYS_DISP_OVL0_MOUT_CLOCK, mmsys_disp_ovl0_mout_clock, mm_sel,
	     mmsys_dummy_cg_con1_regs, 14, CLK_GATE_NO_SETCLR_REG | CLK_GATE_INVERSE),
};

static const struct mtk_gate_regs vdec_gcon_vdec_cken_regs = {
	.set_ofs = 0x0,
	.clr_ofs = 0x4,
	.sta_ofs = 0x0,
};

static const struct mtk_gate_regs vdec_gcon_vdec_larb1_cken_regs = {
	.set_ofs = 0x8,
	.clr_ofs = 0xc,
	.sta_ofs = 0x8,
};

static const struct mtk_gate vdec_gcon_clks[] __initconst = {
	GATE(VDEC_VDEC, vdec_vdec, vdec_sel, vdec_gcon_vdec_cken_regs, 0, CLK_GATE_INVERSE),
	GATE(VDEC_LARB1, vdec_larb1, vdec_sel, vdec_gcon_vdec_larb1_cken_regs, 0, CLK_GATE_INVERSE),
};

static const struct mtk_gate_regs venc_gcon_vencsys_cg_regs = {
	.set_ofs = 0x4,
	.clr_ofs = 0x8,
	.sta_ofs = 0x0,
};

static const struct mtk_gate venc_gcon_clks[] __initconst = {
	/* VENCSYS_CG */
	GATE(VENC_GCON_SET0_LARB, venc_gcon_set0_larb, mm_sel,
	     venc_gcon_vencsys_cg_regs, 0, CLK_GATE_INVERSE),
	GATE(VENC_GCON_SET1_VENC, venc_gcon_set1_venc, venc_sel,
	     venc_gcon_vencsys_cg_regs, 4, CLK_GATE_INVERSE),
	GATE(VENC_GCON_SET2_JPGENC, venc_gcon_set2_jpgenc, venc_sel,
	     venc_gcon_vencsys_cg_regs, 8, CLK_GATE_INVERSE),
	GATE(VENC_GCON_SET3_JPGDEC, venc_gcon_set3_jpgdec, venc_sel,
	     venc_gcon_vencsys_cg_regs, 12, CLK_GATE_INVERSE),
};

static const struct mtk_gate_regs aud_cg_regs = {
	.set_ofs = 0x0000,
	.clr_ofs = 0x0000,
	.sta_ofs = 0x0000,
};

static const struct mtk_gate audio_clks[] __initconst = {
	GATE(AUDIO_TML, audio_tml, audio_sel, aud_cg_regs, 27, CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_DAC_PREDIS, audio_dac_predis, audio_sel, aud_cg_regs, 26,
	     CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_DAC, audio_dac, audio_sel, aud_cg_regs, 25, CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_ADC, audio_adc, audio_sel, aud_cg_regs, 24, CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_APLL_TUNER, audio_apll_tuner, aud_1_sel, aud_cg_regs, 19,
	     CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_APLL2_TUNER, audio_apll2_tuner, aud_2_sel, aud_cg_regs, 18,
	     CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_24M, audio_24m, aud_1_sel, aud_cg_regs, 9, CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_22M, audio_22m, aud_2_sel, aud_cg_regs, 8, CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_I2S, audio_i2s, NULL, aud_cg_regs, 6, CLK_GATE_NO_SETCLR_REG),
	GATE(AUDIO_AFE, audio_afe, audio_sel, aud_cg_regs, 2, CLK_GATE_NO_SETCLR_REG),
};

struct mtk_pll {
	int id;
	const char *name;
	const char *parent_name;
	uint32_t reg;
	uint32_t pwr_reg;
	uint32_t en_mask;
	unsigned int flags;
	const struct clk_ops *ops;
};

#define PLL(_id, _name, _parent, _reg, _pwr_reg, _en_mask, _flags, _ops) { \
		.id = _id,						\
		.name = _name,						\
		.parent_name = _parent,					\
		.reg = _reg,						\
		.pwr_reg = _pwr_reg,					\
		.en_mask = _en_mask,					\
		.flags = _flags,					\
		.ops = _ops,						\
	}

static const struct mtk_pll plls[] __initconst = {
	PLL(APMIXED_ARMPLL_LL, armpll_ll, clk26m, 0x0200, 0x020C, 0xf0000101, HAVE_PLL_HP,
	    &mt_clk_arm_pll_ops),
	PLL(APMIXED_ARMPLL_L, armpll_l, clk26m, 0x0210, 0x021C, 0x00000101, HAVE_PLL_HP,
	    &mt_clk_arm_pll_ops),
	PLL(APMIXED_CCIPLL, ccipll, clk26m, 0x0290, 0x029C, 0x00000101, HAVE_PLL_HP,
	    &mt_clk_arm_pll_ops),
	PLL(APMIXED_MAINPLL, mainpll, clk26m, 0x0220, 0x022C, 0xf0000101, HAVE_PLL_HP,
	    &mt_clk_sdm_pll_ops),
	PLL(APMIXED_UNIVPLL, univpll, clk26m, 0x0230, 0x023C, 0xfe000101, HAVE_FIX_FRQ,
	    &mt_clk_univ_pll_ops),
	PLL(APMIXED_MSDCPLL, msdcpll, clk26m, 0x0250, 0x025C, 0x00000101, HAVE_PLL_HP,
	    &mt_clk_sdm_pll_ops),
	PLL(APMIXED_VENCPLL, vencpll, clk26m, 0x0260, 0x026C, 0x00000101, HAVE_PLL_HP,
	    &mt_clk_sdm_pll_ops),
	PLL(APMIXED_MMPLL, mmpll, clk26m, 0x0240, 0x024C, 0x00000101, HAVE_PLL_HP,
	    &mt_clk_mm_pll_ops),
	PLL(APMIXED_TVDPLL, tvdpll, clk26m, 0x0270, 0x027C, 0xc0000101, HAVE_PLL_HP,
	    &mt_clk_sdm_pll_ops),
	PLL(APMIXED_APLL1, apll1, clk26m, 0x02A0, 0x02B0, 0x00000101, HAVE_FIX_FRQ,
	    &mt_clk_aud_pll_ops),
	PLL(APMIXED_APLL2, apll2, clk26m, 0x02B4, 0x02C4, 0x00000101, HAVE_FIX_FRQ,
	    &mt_clk_aud_pll_ops),
	PLL(SCP_OSCPLL, oscpll, clk26m, 0x0458, 0x0458, 0x00000001, HAVE_FIX_FRQ,
	    &mt_clk_spm_pll_ops),
};

/* =====================auto-end======================== */



#ifdef CONFIG_OF
void __iomem *apmixed_base;
void __iomem *cksys_base;
void __iomem *infracfg_base;
void __iomem *audio_base;
void __iomem *mfgcfg_base;
void __iomem *mmsys_config_base;
void __iomem *img_base;
void __iomem *vdec_gcon_base;
void __iomem *venc_gcon_base;
void __iomem *camsys_base;

/*PLL INIT*/
#define AP_PLL_CON3				(apmixed_base + 0x000C)
#define AP_PLL_CON4				(apmixed_base + 0x0010)
#define MSDCPLL_CON0            (apmixed_base + 0x250)
#define MSDCPLL_PWR_CON0        (apmixed_base + 0x25C)
#define TVDPLL_CON0             (apmixed_base + 0x270)
#define TVDPLL_PWR_CON0         (apmixed_base + 0x27C)
#define APLL1_CON0              (apmixed_base + 0x2A0)
#define APLL1_PWR_CON0          (apmixed_base + 0x2B0)
#define APLL2_CON0              (apmixed_base + 0x2B4)
#define APLL2_PWR_CON0          (apmixed_base + 0x2C4)

/* CG */
#define INFRA_PDN_SET0          (infracfg_base + 0x0080)
#define INFRA_PDN_SET1          (infracfg_base + 0x0088)
#define INFRA_PDN_SET2          (infracfg_base + 0x00A4)
#define MFG_CG_SET              (mfgcfg_base + 0x4)
#define IMG_CG_SET              (img_base + 0x0004)
#define MM_CG_SET0            (mmsys_config_base + 0x104)
#define MM_CG_SET1            (mmsys_config_base + 0x114)
#define MM_DUMMY_CG_SET0            (mmsys_config_base + 0x8a8)
#define MM_DUMMY_CG_SET1            (mmsys_config_base + 0x8ac)
#define VDEC_CKEN_CLR           (vdec_gcon_base + 0x0004)
#define LARB_CKEN_CLR           (vdec_gcon_base + 0x000C)
#define VENC_CG_CLR				(venc_gcon_base + 0x8)
#define AUDIO_TOP_CON0          (audio_base + 0x0000)
#define CAMSYS_CG_SET			(camsys_base + 0x0004)

#ifdef MT_CCF_BRINGUP
#define MFG_CG_CLR              (mfgcfg_base + 0x8)
#define IMG_CG_CLR              (img_base + 0x0008)
#define CAMSYS_CG_CLR			(camsys_base + 0x0008)
#define MM_CG_CLR0            (mmsys_config_base + 0x108)
#define MM_CG_CLR1            (mmsys_config_base + 0x118)
#define VDEC_CKEN_SET		(vdec_gcon_base + 0x0000)
#define LARB_CKEN_SET		(vdec_gcon_base + 0x0008)
#define VENC_CG_SET            (venc_gcon_base + 0x4)

#define MFG_DISABLE_CG	0x1
#define IMG_DISABLE_CG	0x3C43
#define CAMSYS_DISABLE_CG	0x1FC1
#define MM_DISABLE_CG0	0xFFFFBFFF /*bit no use: 14*/
#define MM_DISABLE_CG1 0x00004FFF
#define VDEC_DISABLE_CG	0x1
#define LARB_DISABLE_CG	0x1
#define VENC_DISABLE_CG 0x1111
#define AUDIO_DISABLE_CG 0x800c4000
#endif
#endif

#define INFRA0_CG  0xC7BFBF90	/*0: Disable  ( with clock), 1: Enable ( without clock ) , Gate dummy CG*/
#define INFRA1_CG  0x7A6FC876	/*0: Disable  ( with clock), 1: Enable ( without clock ) , Gate dummy CG*/
#define INFRA2_CG  0xFFFFFEE3	/*0: Disable  ( with clock), 1: Enable ( without clock ) , Gate dummy CG*/
#define AUD_0_CG   0x0F0C0304
#define MFG_CG     0x00000001
#define MM_0_CG   0xFFFFFFF8
#define MM_1_CG   0x0003FC3F
#define MM_DUMMY_0_CG   0xFFFFBFF8	/*function on off */
#define MM_DUMMY_1_CG   0x0000403F	/*function on off */
#define IMG_CG     0x00003C43
#define VDEC_CG    0x00000001	/*CLR to Gating*/
#define LARB_CG    0x00000001	/*CLR to Gating*/
#define VENC_CG    0x00001111	/*CLR to Gating*/
#define CAM_CG	   0x00001FC1

#define CG_BOOTUP_PDN 1



static const struct mtk_fixed_factor root_clk_alias[] __initconst = {
};


static void __init init_clk_apmixedsys(void __iomem *apmixed_base, void __iomem *spm_base,
				       struct clk_onecell_data *clk_data)
{
	int i;
	struct clk *clk;

	for (i = 0; i < ARRAY_SIZE(plls); i++) {
		const struct mtk_pll *pll = &plls[i];

		if (!strcmp("oscpll", pll->name)) {
			clk = mtk_clk_register_pll(pll->name, pll->parent_name,
			spm_base + pll->reg,
			spm_base + pll->pwr_reg,
			pll->en_mask, pll->flags, pll->ops);
		} else {
			clk = mtk_clk_register_pll(pll->name, pll->parent_name,
			apmixed_base + pll->reg,
			apmixed_base + pll->pwr_reg,
			pll->en_mask, pll->flags, pll->ops);
		}
		if (IS_ERR(clk)) {
			pr_debug("Failed to register clk %s: %ld\n", pll->name, PTR_ERR(clk));
			continue;
		}

		if (clk_data)
			clk_data->clks[pll->id] = clk;

#if MT_CCF_DEBUG
		pr_debug("[CCF] pll %3d: %s\n", i, pll->name);
#endif				/* MT_CCF_DEBUG */
	}
}

static void __init init_clk_gates(void __iomem *reg_base,
					const struct mtk_gate *clks,
					int num,
					struct clk_onecell_data *clk_data)
{
	int i;
	struct clk *clk;

	for (i = 0; i < num; i++) {
		const struct mtk_gate *gate = &clks[i];

#if defined(CONFIG_MACH_MT6757)
		clk = mtk_clk_register_gate(gate->name, gate->parent_name,
						reg_base,
						gate->regs->set_ofs,
						gate->regs->clr_ofs,
						gate->regs->sta_ofs,
						gate->shift,
						gate->ops,
						gate->flags);
#else
		clk = mtk_clk_register_gate(gate->name, gate->parent_name,
						reg_base,
						gate->regs->set_ofs,
						gate->regs->clr_ofs,
						gate->regs->sta_ofs,
						gate->shift,
						/* gate->flags, */
						gate->ops);
#endif

		if (IS_ERR(clk)) {
			pr_debug("Failed to register clk %s: %ld\n", gate->name, PTR_ERR(clk));
			continue;
		}

		if (clk_data)
			clk_data->clks[gate->id] = clk;

#if MT_CCF_DEBUG
		pr_debug("[CCF] gate %3d: %s\n", i, gate->name);
#endif				/* MT_CCF_DEBUG */
	}
}

static void __init init_clk_infrasys(void __iomem *infrasys_base,
				     struct clk_onecell_data *clk_data)
{
	pr_debug("[CCF] init infrasys gates:\n");
	init_clk_gates(infrasys_base, infracfg_ao_clks, ARRAY_SIZE(infracfg_ao_clks), clk_data);
}

static void __init init_clk_mfgsys(void __iomem *mfgsys_base, struct clk_onecell_data *clk_data)
{
	pr_debug("[CCF] init mfgsys gates:\n");
	init_clk_gates(mfgsys_base, mfgcfg_clks, ARRAY_SIZE(mfgcfg_clks), clk_data);
}

static void __init init_clk_imgsys(void __iomem *imgsys_base, struct clk_onecell_data *clk_data)
{
	pr_debug("[CCF] init imgsys gates:\n");
	init_clk_gates(imgsys_base, img_clks, ARRAY_SIZE(img_clks), clk_data);
}

static void __init init_clk_camsys(void __iomem *camsys_base, struct clk_onecell_data *clk_data)
{
	pr_debug("[CCF] init camsys gates:\n");
	init_clk_gates(camsys_base, camsys_clks, ARRAY_SIZE(camsys_clks), clk_data);
}

static void __init init_clk_mmsys(void __iomem *mmsys_base, struct clk_onecell_data *clk_data)
{
	pr_debug("[CCF] init mmsys gates:\n");
	init_clk_gates(mmsys_base, mmsys_config_clks, ARRAY_SIZE(mmsys_config_clks), clk_data);
}

static void __init init_clk_vdecsys(void __iomem *vdecsys_base, struct clk_onecell_data *clk_data)
{
	pr_debug("[CCF] init vdecsys gates:\n");
	init_clk_gates(vdecsys_base, vdec_gcon_clks, ARRAY_SIZE(vdec_gcon_clks), clk_data);
}

static void __init init_clk_vencsys(void __iomem *vencsys_base, struct clk_onecell_data *clk_data)
{
	pr_debug("[CCF] init vencsys gates:\n");
	init_clk_gates(vencsys_base, venc_gcon_clks, ARRAY_SIZE(venc_gcon_clks), clk_data);
}

static void __init init_clk_audiosys(void __iomem *audiosys_base,
				     struct clk_onecell_data *clk_data)
{
	pr_debug("[CCF] init audiosys gates:\n");
	init_clk_gates(audiosys_base, audio_clks, ARRAY_SIZE(audio_clks), clk_data);
}

/*
 * device tree support
 */

static struct clk_onecell_data *alloc_clk_data(unsigned int clk_num)
{
	int i;
	struct clk_onecell_data *clk_data;

	clk_data = kzalloc(sizeof(*clk_data), GFP_KERNEL);
	if (!clk_data)
		return NULL;

	clk_data->clks = kcalloc(clk_num, sizeof(struct clk *), GFP_KERNEL);
	if (!clk_data->clks) {
		kfree(clk_data);
		return NULL;
	}

	clk_data->clk_num = clk_num;

	for (i = 0; i < clk_num; ++i)
		clk_data->clks[i] = ERR_PTR(-ENOENT);

	return clk_data;
}

static void __iomem *get_reg(struct device_node *np, int index)
{
#if DUMMY_REG_TEST
	return kzalloc(PAGE_SIZE, GFP_KERNEL);
#else
	return of_iomap(np, index);
#endif
}

extern void mtk_clk_register_factors(const struct mtk_fixed_factor *clks,
	int num, struct clk_onecell_data *clk_data);
static void __init mt_topckgen_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	pr_debug("[CCF] %s: %s\n", __func__, node->name);

	base = get_reg(node, 0);
	if (!base) {
		pr_debug("ioremap topckgen failed\n");
		return;
	}

	clk_data = alloc_clk_data(TOP_NR_CLK);

	//init_clk_root_alias(clk_data);
	mtk_clk_register_factors(root_clk_alias,
			ARRAY_SIZE(root_clk_alias), clk_data);
	//init_clk_top_div(clk_data);
	mtk_clk_register_factors(top_divs, ARRAY_SIZE(top_divs), clk_data);

	mtk_clk_register_mux_clr_set_upds(top_muxes, ARRAY_SIZE(top_muxes), base,
					  &mt6757_clk_lock, clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_debug("could not register clock provide\n");

#if CG_BOOTUP_PDN
	cksys_base = base;
#endif
}

CLK_OF_DECLARE_DRIVER(mtk_topckgen, "mediatek,topckgen", mt_topckgen_init);

#define PLL_EN (0x1 << 0)
#define PLL_PWR_ON  (0x1 << 0)
#define PLL_ISO_EN  (0x1 << 1)
static void __init mt_apmixedsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	void __iomem *spm_base;
	int r;

	pr_debug("[CCF] %s: %s\n", __func__, node->name);

	base = get_reg(node, 0);
	spm_base = get_reg(node, 1);
	if ((!base) || (!spm_base)) {
		pr_debug("ioremap apmixedsys/spm failed\n");
		return;
	}

	clk_data = alloc_clk_data(APMIXED_NR_CLK);

	init_clk_apmixedsys(base, spm_base, clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_debug("could not register clock provide\n");

	apmixed_base = base;

	clk_writel(AP_PLL_CON3, 0x7D555C);	/* L, LL and TDCLK SW Mode */
	clk_writel(AP_PLL_CON4, 0x2005);	/* others HW Mode */

	/*disable */
#if CG_BOOTUP_PDN
/* TODO: remove to LK after boot PDN enable */
/* MSDCPLL */
	clk_clrl(MSDCPLL_CON0, PLL_EN);
	clk_setl(MSDCPLL_PWR_CON0, PLL_ISO_EN);
	clk_clrl(MSDCPLL_PWR_CON0, PLL_PWR_ON);
/*TVDPLL*/
	clk_clrl(TVDPLL_CON0, PLL_EN);
	clk_setl(TVDPLL_PWR_CON0, PLL_ISO_EN);
	clk_clrl(TVDPLL_PWR_CON0, PLL_PWR_ON);
/*APLL 1,2 */
	clk_clrl(APLL1_CON0, PLL_EN);
	clk_setl(APLL1_PWR_CON0, PLL_ISO_EN);
	clk_clrl(APLL1_PWR_CON0, PLL_PWR_ON);

	clk_clrl(APLL2_CON0, PLL_EN);
	clk_setl(APLL2_PWR_CON0, PLL_ISO_EN);
	clk_clrl(APLL2_PWR_CON0, PLL_PWR_ON);

#endif
}

CLK_OF_DECLARE_DRIVER(mtk_apmixedsys, "mediatek,apmixed", mt_apmixedsys_init);

static void __init mt_infrasys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	pr_debug("[CCF] %s: %s\n", __func__, node->name);

	base = get_reg(node, 0);
	if (!base) {
		pr_debug("ioremap infrasys failed\n");
		return;
	}

	clk_data = alloc_clk_data(INFRACFG_AO_NR_CLK);

	init_clk_infrasys(base, clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_debug("could not register clock provide\n");

#if CG_BOOTUP_PDN
	infracfg_base = base;
	clk_writel(INFRA_PDN_SET0, INFRA0_CG);
	clk_writel(INFRA_PDN_SET1, INFRA1_CG);
	clk_writel(INFRA_PDN_SET2, INFRA2_CG);
#endif
}

CLK_OF_DECLARE_DRIVER(mtk_infrasys, "mediatek,infracfg_ao", mt_infrasys_init);

static void __init mt_mfgsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	pr_debug("[CCF] %s: %s\n", __func__, node->name);

	base = get_reg(node, 0);
	if (!base) {
		pr_debug("ioremap mfgsys failed\n");
		return;
	}

	clk_data = alloc_clk_data(MFGCFG_NR_CLK);

	init_clk_mfgsys(base, clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_debug("could not register clock provide\n");

	mfgcfg_base = base;

#if MT_CCF_BRINGUP
	clk_writel(MFG_CG_CLR, MFG_DISABLE_CG);
#endif
#if CG_BOOTUP_PDN
	clk_writel(MFG_CG_SET, MFG_CG);
#endif

}

CLK_OF_DECLARE_DRIVER(mtk_mfgsys, "mediatek,g3d_config", mt_mfgsys_init);

static void __init mt_imgsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	pr_debug("[CCF] %s: %s\n", __func__, node->name);

	base = get_reg(node, 0);
	if (!base) {
		pr_debug("ioremap imgsys failed\n");
		return;
	}

	clk_data = alloc_clk_data(IMG_CLK);

	init_clk_imgsys(base, clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_debug("could not register clock provide\n");

	img_base = base;

#if MT_CCF_BRINGUP
	clk_writel(IMG_CG_CLR, IMG_DISABLE_CG);
#endif
#if CG_BOOTUP_PDN
	clk_writel(IMG_CG_SET, IMG_CG);
#endif


}

CLK_OF_DECLARE_DRIVER(mtk_imgsys, "mediatek,imgsys_config", mt_imgsys_init);

static void __init mt_camsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	pr_debug("[CCF] %s: %s\n", __func__, node->name);

	base = get_reg(node, 0);
	if (!base) {
		pr_debug("ioremap camsys failed\n");
		return;
	}

	clk_data = alloc_clk_data(CAMSYS_NR_CLK);

	init_clk_camsys(base, clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_debug("could not register clock provide\n");

	camsys_base = base;

#if MT_CCF_BRINGUP
	clk_writel(CAMSYS_CG_CLR, CAMSYS_DISABLE_CG);
#endif
#if CG_BOOTUP_PDN
	clk_writel(CAMSYS_CG_SET, CAM_CG);
#endif

}

CLK_OF_DECLARE_DRIVER(mtk_camsys, "mediatek,camsys_config", mt_camsys_init);

#define MMSYS_CG_SET0_OFS 0x104
#define MMSYS_CG_SET1_OFS 0x114
static void __init mt_mmsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	pr_debug("[CCF] %s: %s\n", __func__, node->name);

	base = get_reg(node, 0);
	if (!base) {
		pr_debug("ioremap mmsys failed\n");
		return;
	}

	clk_data = alloc_clk_data(MMSYS_CONFIG_NR_CLK);

	init_clk_mmsys(base, clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_debug("could not register clock provide\n");

	mmsys_config_base = base;

#if MT_CCF_BRINGUP
	clk_writel(MM_CG_CLR0, MM_DISABLE_CG0);
	clk_writel(MM_CG_CLR1, MM_DISABLE_CG1);
#endif
#if CG_BOOTUP_PDN
	clk_writel(MM_CG_SET0, MM_0_CG);
	clk_writel(MM_CG_SET1, MM_1_CG);
	clk_writel(MM_DUMMY_CG_SET0, ~MM_DUMMY_0_CG);
	clk_writel(MM_DUMMY_CG_SET1, ~MM_DUMMY_1_CG);
#endif

}

CLK_OF_DECLARE_DRIVER(mtk_mmsys, "mediatek,mmsys_config", mt_mmsys_init);

static void __init mt_vdecsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	pr_debug("[CCF] %s: %s\n", __func__, node->name);

	base = get_reg(node, 0);
	if (!base) {
		pr_debug("ioremap vdecsys failed\n");
		return;
	}

	clk_data = alloc_clk_data(VDEC_GCON_NR_CLK);

	init_clk_vdecsys(base, clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_debug("could not register clock provide\n");

	vdec_gcon_base = base;

#if MT_CCF_BRINGUP
	clk_writel(VDEC_CKEN_SET, VDEC_DISABLE_CG);
	clk_writel(LARB_CKEN_SET, LARB_DISABLE_CG);
#endif
#if CG_BOOTUP_PDN
	clk_writel(VDEC_CKEN_CLR, VDEC_CG);
	clk_writel(LARB_CKEN_CLR, LARB_CG);
#endif

}

CLK_OF_DECLARE_DRIVER(mtk_vdecsys, "mediatek,vdec_gcon", mt_vdecsys_init);

static void __init mt_vencsys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	pr_debug("[CCF] %s: %s\n", __func__, node->name);

	base = get_reg(node, 0);
	if (!base) {
		pr_debug("ioremap vencsys failed\n");
		return;
	}

	clk_data = alloc_clk_data(VENC_GCON_NR_CLK);

	init_clk_vencsys(base, clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_debug("could not register clock provide\n");

	venc_gcon_base = base;

#if MT_CCF_BRINGUP
	clk_clrl(VENC_CG_SET, VENC_DISABLE_CG);
#endif
#if CG_BOOTUP_PDN
	clk_clrl(VENC_CG_CLR, VENC_CG);
#endif

}

CLK_OF_DECLARE_DRIVER(mtk_vencsys, "mediatek,venc_gcon", mt_vencsys_init);

static void __init mt_audiosys_init(struct device_node *node)
{
	struct clk_onecell_data *clk_data;
	void __iomem *base;
	int r;

	pr_debug("[CCF] %s: %s\n", __func__, node->name);

	base = get_reg(node, 0);
	if (!base) {
		pr_debug("ioremap audiosys failed\n");
		return;
	}

	clk_data = alloc_clk_data(AUDIO_NR_CLK);

	init_clk_audiosys(base, clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);
	if (r)
		pr_debug("could not register clock provide\n");

	audio_base = base;

#if MT_CCF_BRINGUP
	clk_writel(AUDIO_TOP_CON0, AUDIO_DISABLE_CG);
#endif
#if CG_BOOTUP_PDN
	clk_writel(AUDIO_TOP_CON0, clk_readl(AUDIO_TOP_CON0) | AUD_0_CG);
#endif

}

CLK_OF_DECLARE_DRIVER(mtk_audiosys, "mediatek,audio", mt_audiosys_init);

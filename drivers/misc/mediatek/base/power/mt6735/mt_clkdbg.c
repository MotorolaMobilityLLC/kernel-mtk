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

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>

#include <linux/clk-provider.h>

#include <mt-plat/sync_write.h>

/* #define CLKDBG_DISABLE_UNUSED	1 */
/* #define DUMMY_REG_TEST		0 */

#define TAG	"[clkdbg] "

#define clk_err(fmt, args...)	pr_err(TAG fmt, ##args)
#define clk_warn(fmt, args...)	pr_warn(TAG fmt, ##args)
#define clk_info(fmt, args...)	pr_debug(TAG fmt, ##args)
#define clk_dbg(fmt, args...)	pr_debug(TAG fmt, ##args)
#define clk_ver(fmt, args...)	pr_debug(TAG fmt, ##args)

/************************************************
 **********      register access       **********
 ************************************************/

#ifndef BIT
#define BIT(_bit_)		(u32)(1U << (_bit_))
#endif

#ifndef GENMASK
#define GENMASK(h, l)	(((1U << ((h) - (l) + 1)) - 1) << (l))
#endif

#define ALT_BITS(o, h, l, v) \
	(((o) & ~GENMASK(h, l)) | (((v) << (l)) & GENMASK(h, l)))

#define clk_readl(addr)		readl(addr)
#define clk_writel(addr, val)	do { writel(val, addr); mb(); } while (0)
#define clk_setl(addr, val)	clk_writel(addr, clk_readl(addr) | (val))
#define clk_clrl(addr, val)	clk_writel(addr, clk_readl(addr) & ~(val))
#define clk_writel_mask(addr, h, l, v)	\
	clk_writel(addr, (clk_readl(addr) & ~GENMASK(h, l)) | ((v) << (l)))

#define ABS_DIFF(a, b)	((a) > (b) ? (a) - (b) : (b) - (a))

/* ROOT */
#define clk_null		"clk_null"
#define clk26m			"clk26m"
#define clk32k			"clk32k"

#define clkph_mck_o		"clkph_mck_o"
#define dpi_ck			"dpi_ck"

/* PLL */
#define armpll		"armpll"
#define mainpll		"mainpll"
#define msdcpll		"msdcpll"
#define univpll		"univpll"
#define mmpll		"mmpll"
#define vencpll		"vencpll"
#define tvdpll		"tvdpll"
#define apll1		"apll1"
#define apll2		"apll2"

/* DIV */
#define ad_apll1_ck		"ad_apll1_ck"
#define ad_sys_26m_ck		"ad_sys_26m_ck"
#define ad_sys_26m_d2		"ad_sys_26m_d2"
#define dmpll_ck		"dmpll_ck"
#define dmpll_d2		"dmpll_d2"
#define dmpll_d4		"dmpll_d4"
#define dmpll_d8		"dmpll_d8"
#define dpi_ck		"dpi_ck"
#define mmpll_ck		"mmpll_ck"
#define msdcpll_ck		"msdcpll_ck"
#define msdcpll_d16		"msdcpll_d16"
#define msdcpll_d2		"msdcpll_d2"
#define msdcpll_d4		"msdcpll_d4"
#define msdcpll_d8		"msdcpll_d8"
#define syspll_d2		"syspll_d2"
#define syspll_d3		"syspll_d3"
#define syspll_d5		"syspll_d5"
#define syspll1_d16		"syspll1_d16"
#define syspll1_d2		"syspll1_d2"
#define syspll1_d4		"syspll1_d4"
#define syspll1_d8		"syspll1_d8"
#define syspll2_d2		"syspll2_d2"
#define syspll2_d4		"syspll2_d4"
#define syspll3_d2		"syspll3_d2"
#define syspll3_d4		"syspll3_d4"
#define syspll4_d2		"syspll4_d2"
#define syspll4_d2_d8		"syspll4_d2_d8"
#define syspll4_d4		"syspll4_d4"
#define tvdpll_ck		"tvdpll_ck"
#define tvdpll_d2		"tvdpll_d2"
#define tvdpll_d4		"tvdpll_d4"
#define univpll_d2		"univpll_d2"
#define univpll_d26		"univpll_d26"
#define univpll_d3		"univpll_d3"
#define univpll_d5		"univpll_d5"
#define univpll1_d2		"univpll1_d2"
#define univpll1_d4		"univpll1_d4"
#define univpll1_d8		"univpll1_d8"
#define univpll2_d2		"univpll2_d2"
#define univpll2_d4		"univpll2_d4"
#define univpll2_d8		"univpll2_d8"
#define univpll3_d2		"univpll3_d2"
#define univpll3_d4		"univpll3_d4"
#define vencpll_ck		"vencpll_ck"
#define vencpll_d3		"vencpll_d3"
#define whpll_audio_ck		"whpll_audio_ck"

/* TOP */
#define axi_sel			"axi_sel"
#define mem_sel			"mem_sel"
#define ddrphycfg_sel			"ddrphycfg_sel"
#define mm_sel			"mm_sel"
#define pwm_sel			"pwm_sel"
#define vdec_sel			"vdec_sel"
#define mfg_sel			"mfg_sel"
#define camtg_sel			"camtg_sel"
#define uart_sel			"uart_sel"
#define spi_sel			"spi_sel"
#define usb20_sel			"usb20_sel"
#define msdc50_0_sel			"msdc50_0_sel"
#define msdc30_0_sel			"msdc30_0_sel"
#define msdc30_1_sel			"msdc30_1_sel"
#define msdc30_2_sel			"msdc30_2_sel"
#define msdc30_3_sel			"msdc30_3_sel"
#define audio_sel			"audio_sel"
#define aud_intbus_sel			"aud_intbus_sel"
#define pmicspi_sel			"pmicspi_sel"
#define scp_sel			"scp_sel"
#define atb_sel			"atb_sel"
#define dpi0_sel			"dpi0_sel"
#define scam_sel			"scam_sel"
#define mfg13m_sel			"mfg13m_sel"
#define aud_1_sel			"aud_1_sel"
#define aud_2_sel			"aud_2_sel"
#define irda_sel			"irda_sel"
#define irtx_sel			"irtx_sel"
#define disppwm_sel			"disppwm_sel"

/* INFRA */
#define infra_dbgclk		"infra_dbgclk"
#define infra_gce		"infra_gce"
#define infra_trbg		"infra_trbg"
#define infra_cpum		"infra_cpum"
#define infra_devapc		"infra_devapc"
#define infra_audio		"infra_audio"
#define infra_gcpu		"infra_gcpu"
#define infra_l2csram		"infra_l2csram"
#define infra_m4u		"infra_m4u"
#define infra_cldma		"infra_cldma"
#define infra_connmcubus	"infra_connmcubus"
#define infra_kp		"infra_kp"
#define infra_apxgpt		"infra_apxgpt"
#define infra_sej		"infra_sej"
#define infra_ccif0ap		"infra_ccif0ap"
#define infra_ccif1ap		"infra_ccif1ap"
#define infra_pmicspi		"infra_pmicspi"
#define infra_pmicwrap		"infra_pmicwrap"

/* PERI */
#define peri_disp_pwm		"peri_disp_pwm"
#define peri_therm		"peri_therm"
#define peri_pwm1		"peri_pwm1"
#define peri_pwm2		"peri_pwm2"
#define peri_pwm3		"peri_pwm3"
#define peri_pwm4		"peri_pwm4"
#define peri_pwm5		"peri_pwm5"
#define peri_pwm6		"peri_pwm6"
#define peri_pwm7		"peri_pwm7"
#define peri_pwm		"peri_pwm"
#define peri_usb0		"peri_usb0"
#define peri_irda		"peri_irda"
#define peri_apdma		"peri_apdma"
#define peri_msdc30_0		"peri_msdc30_0"
#define peri_msdc30_1		"peri_msdc30_1"
#define peri_msdc30_2		"peri_msdc30_2"
#define peri_msdc30_3		"peri_msdc30_3"
#define peri_uart0		"peri_uart0"
#define peri_uart1		"peri_uart1"
#define peri_uart2		"peri_uart2"
#define peri_uart3		"peri_uart3"
#define peri_uart4		"peri_uart4"
#define peri_btif		"peri_btif"
#define peri_i2c0		"peri_i2c0"
#define peri_i2c1		"peri_i2c1"
#define peri_i2c2		"peri_i2c2"
#define peri_i2c3		"peri_i2c3"
#define peri_auxadc		"peri_auxadc"
#define peri_spi0		"peri_spi0"
#define peri_irtx		"peri_irtx"

/* MFG */
#define mfg_bg3d			"mfg_bg3d"

/* IMG */
#define img_image_larb2_smi		"img_image_larb2_smi"
#define img_image_cam_smi		"img_image_cam_smi"
#define img_image_cam_cam		"img_image_cam_cam"
#define img_image_sen_tg		"img_image_sen_tg"
#define img_image_sen_cam		"img_image_sen_cam"
#define img_image_cam_sv		"img_image_cam_sv"
#define img_image_sufod		"img_image_sufod"
#define img_image_fd		"img_image_fd"

/* MM_SYS */
#define mm_disp0_smi_common		"mm_disp0_smi_common"
#define mm_disp0_smi_larb0		"mm_disp0_smi_larb0"
#define mm_disp0_cam_mdp		"mm_disp0_cam_mdp"
#define mm_disp0_mdp_rdma		"mm_disp0_mdp_rdma"
#define mm_disp0_mdp_rsz0		"mm_disp0_mdp_rsz0"
#define mm_disp0_mdp_rsz1		"mm_disp0_mdp_rsz1"
#define mm_disp0_mdp_tdshp		"mm_disp0_mdp_tdshp"
#define mm_disp0_mdp_wdma		"mm_disp0_mdp_wdma"
#define mm_disp0_mdp_wrot		"mm_disp0_mdp_wrot"
#define mm_disp0_fake_eng		"mm_disp0_fake_eng"
#define mm_disp0_disp_ovl0		"mm_disp0_disp_ovl0"
#define mm_disp0_disp_rdma0		"mm_disp0_disp_rdma0"
#define mm_disp0_disp_rdma1		"mm_disp0_disp_rdma1"
#define mm_disp0_disp_wdma0		"mm_disp0_disp_wdma0"
#define mm_disp0_disp_color		"mm_disp0_disp_color"
#define mm_disp0_disp_ccorr		"mm_disp0_disp_ccorr"
#define mm_disp0_disp_aal		"mm_disp0_disp_aal"
#define mm_disp0_disp_gamma		"mm_disp0_disp_gamma"
#define mm_disp0_disp_dither		"mm_disp0_disp_dither"
#define mm_disp1_dsi_engine		"mm_disp1_dsi_engine"
#define mm_disp1_dsi_digital		"mm_disp1_dsi_digital"
#define mm_disp1_dpi_engine		"mm_disp1_dpi_engine"
#define mm_disp1_dpi_pixel		"mm_disp1_dpi_pixel"

/* VDEC */
#define vdec0_vdec		"vdec0_vdec"
#define vdec1_larb		"vdec1_larb"

/* VENC */
#define venc_larb		"venc_larb"
#define venc_venc		"venc_venc"
#define venc_jpgenc		"venc_jpgenc"
#define venc_jpgdec		"venc_jpgdec"

/* AUDIO */
#define audio_afe		"audio_afe"
#define audio_i2s		"audio_i2s"
#define audio_22m		"audio_22m"
#define audio_24m		"audio_24m"
#define audio_apll2_tuner		"audio_apll2_tuner"
#define audio_apll_tuner		"audio_apll_tuner"
#define audio_adc		"audio_adc"
#define audio_dac		"audio_dac"
#define audio_dac_predis		"audio_dac_predis"
#define audio_tml		"audio_tml"

/* SCP */
#define pg_md1			"pg_md1"
#define pg_md2			"pg_md2"
#define pg_conn			"pg_conn"
#define pg_dis			"pg_dis"
#define pg_mfg			"pg_mfg"
#define pg_isp			"pg_isp"
#define pg_vde			"pg_vde"
#define pg_ven			"pg_ven"

static void dump_clk_state(const char *clkname, struct seq_file *s)
{
	struct clk *c = __clk_lookup(clkname);
	struct clk *p = IS_ERR_OR_NULL(c) ? NULL : __clk_get_parent(c);

	if (IS_ERR_OR_NULL(c)) {
		seq_printf(s, "[%17s: NULL]\n", clkname);
		return;
	}

	seq_printf(s, "[%-17s: %3s, %3d, %3d, %10ld, %17s]\n",
		__clk_get_name(c),
		__clk_is_enabled(c) ? "ON" : "off",
		__clk_get_prepare_count(c),
		__clk_get_enable_count(c),
		__clk_get_rate(c),
		p ? __clk_get_name(p) : "");
}

static int clkdbg_dump_state_all(struct seq_file *s, void *v)
{
	static const char * const clks[] = {
		/* ROOT */
		clk_null,
		clk26m,
		clk32k,

		/* PLL */
		armpll,
		mainpll,
		msdcpll,
		univpll,
		mmpll,
		vencpll,
		tvdpll,
		apll1,
		apll2,
		/* DIV */
		ad_apll1_ck,
		ad_sys_26m_ck,
		ad_sys_26m_d2,
		dmpll_ck,
		dmpll_d2,
		dmpll_d4,
		dmpll_d8,
		dpi_ck,
		mmpll_ck,
		msdcpll_ck,
		msdcpll_d16,
		msdcpll_d2,
		msdcpll_d4,
		msdcpll_d8,
		syspll_d2,
		syspll_d3,
		syspll_d5,
		syspll1_d16,
		syspll1_d2,
		syspll1_d4,
		syspll1_d8,
		syspll2_d2,
		syspll2_d4,
		syspll3_d2,
		syspll3_d4,
		syspll4_d2,
		syspll4_d2_d8,
		syspll4_d4,
		tvdpll_ck,
		tvdpll_d2,
		tvdpll_d4,
		univpll_d2,
		univpll_d26,
		univpll_d3,
		univpll_d5,
		univpll1_d2,
		univpll1_d4,
		univpll1_d8,
		univpll2_d2,
		univpll2_d4,
		univpll2_d8,
		univpll3_d2,
		univpll3_d4,
		vencpll_ck,
		vencpll_d3,
		whpll_audio_ck,

		/* TOP */
		axi_sel,
		mem_sel,
		ddrphycfg_sel,
		mm_sel,
		pwm_sel,
		vdec_sel,
		mfg_sel,
		camtg_sel,
		uart_sel,
		spi_sel,
		usb20_sel,
		msdc50_0_sel,
		msdc30_0_sel,
		msdc30_1_sel,
		msdc30_2_sel,
		msdc30_3_sel,
		audio_sel,
		aud_intbus_sel,
		pmicspi_sel,
		scp_sel,
		atb_sel,
		dpi0_sel,
		scam_sel,
		mfg13m_sel,
		aud_1_sel,
		aud_2_sel,
		irda_sel,
		irtx_sel,
		disppwm_sel,

		/* INFRA */
		infra_dbgclk,
		infra_gce,
		infra_trbg,
		infra_cpum,
		infra_devapc,
		infra_audio,
		infra_gcpu,
		infra_l2csram,
		infra_m4u,
		infra_cldma,
		infra_connmcubus,
		infra_kp,
		infra_apxgpt,
		infra_sej,
		infra_ccif0ap,
		infra_ccif1ap,
		infra_pmicspi,
		infra_pmicwrap,

		/* PERI */
		peri_disp_pwm,
		peri_therm,
		peri_pwm1,
		peri_pwm2,
		peri_pwm3,
		peri_pwm4,
		peri_pwm5,
		peri_pwm6,
		peri_pwm7,
		peri_pwm,
		peri_usb0,
		peri_irda,
		peri_apdma,
		peri_msdc30_0,
		peri_msdc30_1,
		peri_msdc30_2,
		peri_msdc30_3,
		peri_uart0,
		peri_uart1,
		peri_uart2,
		peri_uart3,
		peri_uart4,
		peri_btif,
		peri_i2c0,
		peri_i2c1,
		peri_i2c2,
		peri_i2c3,
		peri_auxadc,
		peri_spi0,
		peri_irtx,

		/* MFG */
		mfg_bg3d,

		/* IMG */
		img_image_larb2_smi,
		img_image_cam_smi,
		img_image_cam_cam,
		img_image_sen_tg,
		img_image_sen_cam,
		img_image_cam_sv,
		img_image_sufod,
		img_image_fd,

		/* MM0 */
		mm_disp0_smi_common,
		mm_disp0_smi_larb0,
		mm_disp0_cam_mdp,
		mm_disp0_mdp_rdma,
		mm_disp0_mdp_rsz0,
		mm_disp0_mdp_rsz1,
		mm_disp0_mdp_tdshp,
		mm_disp0_mdp_wdma,
		mm_disp0_mdp_wrot,
		mm_disp0_fake_eng,
		mm_disp0_disp_ovl0,
		mm_disp0_disp_rdma0,
		mm_disp0_disp_rdma1,
		mm_disp0_disp_wdma0,
		mm_disp0_disp_color,
		mm_disp0_disp_ccorr,
		mm_disp0_disp_aal,
		mm_disp0_disp_gamma,
		mm_disp0_disp_dither,

		/* MM1 */
		mm_disp1_dsi_engine,
		mm_disp1_dsi_digital,
		mm_disp1_dpi_engine,
		mm_disp1_dpi_pixel,

		/* VDEC */
		vdec0_vdec,
		vdec1_larb,

		/* VENC */
		venc_larb,
		venc_venc,
		venc_jpgenc,
		venc_jpgdec,

		/* AUDIO */
		audio_afe,
		audio_i2s,
		audio_22m,
		audio_24m,
		audio_apll2_tuner,
		audio_apll_tuner,
		audio_adc,
		audio_dac,
		audio_dac_predis,
		audio_tml,

		pg_md1,
		pg_md2,
		pg_conn,
		pg_dis,
		pg_mfg,
		pg_isp,
		pg_vde,
		pg_ven,
	};

	int i;

	pr_debug("\n");
	for (i = 0; i < ARRAY_SIZE(clks); i++)
		dump_clk_state(clks[i], s);

	return 0;
}

static char last_cmd[128] = "null";

static int clkdbg_prepare_enable(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd) + 1];
	char *c = cmd;
	char *ign;
	char *clk_name;
	struct clk *clk;
	int r;

	strncpy(cmd, last_cmd, sizeof(last_cmd));

	ign = strsep(&c, " ");
	clk_name = strsep(&c, " ");

	seq_printf(s, "clk_prepare_enable(%s): ", clk_name);

	clk = __clk_lookup(clk_name);
	if (IS_ERR_OR_NULL(clk)) {
		seq_printf(s, "clk_get(): 0x%p\n", clk);
		return PTR_ERR(clk);
	}

	r = clk_prepare_enable(clk);
	seq_printf(s, "%d\n", r);

	return r;
}

static int clkdbg_disable_unprepare(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd) + 1];
	char *c = cmd;
	char *ign;
	char *clk_name;
	struct clk *clk;

	strncpy(cmd, last_cmd, sizeof(last_cmd));

	ign = strsep(&c, " ");
	clk_name = strsep(&c, " ");

	seq_printf(s, "clk_disable_unprepare(%s): ", clk_name);

	clk = __clk_lookup(clk_name);
	if (IS_ERR_OR_NULL(clk)) {
		seq_printf(s, "clk_get(): 0x%p\n", clk);
		return PTR_ERR(clk);
	}

	clk_disable_unprepare(clk);
	seq_puts(s, "0\n");

	return 0;
}

#if 0
static int clkdbg_ops_prepare_enable(char *clk_name)
{
	struct clk *clk;
	int r;

	clk = __clk_lookup(clk_name);
	if (IS_ERR_OR_NULL(clk)) {
		clk_warn("[CLKDBG] clk_get(): 0x%p\n", clk);
		return PTR_ERR(clk);
	}

	r = clk_prepare_enable(clk);
	clk_warn("[CLKDBG] clk_prepare_enable(%s): %d\n", clk_name, r);

	return r;
}

static int clkdbg_ops_disable_unprepare(char *clk_name)
{
	struct clk *clk;

	clk = __clk_lookup(clk_name);
	if (IS_ERR_OR_NULL(clk)) {
		clk_warn("[CLKDBG] clk_get(): 0x%p\n", clk);
		return PTR_ERR(clk);
	}

	clk_disable_unprepare(clk);
	clk_warn("[CLKDBG] clk_disable_unprepare(%s)\n", clk_name);

	return 0;
}
#endif

static int clkdbg_set_parent(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd) + 1];
	char *c = cmd;
	char *ign;
	char *clk_name;
	char *parent_name;
	struct clk *clk;
	struct clk *parent;
	int r;

	strncpy(cmd, last_cmd, sizeof(last_cmd));

	ign = strsep(&c, " ");
	clk_name = strsep(&c, " ");
	parent_name = strsep(&c, " ");

	seq_printf(s, "clk_set_parent(%s, %s): ", clk_name, parent_name);

	clk = __clk_lookup(clk_name);
	if (IS_ERR_OR_NULL(clk)) {
		seq_printf(s, "clk_get(): 0x%p\n", clk);
		return PTR_ERR(clk);
	}

	parent = __clk_lookup(parent_name);
	if (IS_ERR_OR_NULL(parent)) {
		seq_printf(s, "clk_get(): 0x%p\n", parent);
		return PTR_ERR(parent);
	}

	r = clk_prepare_enable(clk);
	if (r) {
		seq_printf(s, "clk_prepare_enable() failed: %d\n", r);
		return r;
	}

	r = clk_set_parent(clk, parent);
	clk_disable_unprepare(clk);
	seq_printf(s, "%d\n", r);

	return r;
}

static int clkdbg_set_rate(struct seq_file *s, void *v)
{
	char cmd[sizeof(last_cmd) + 1];
	char *c = cmd;
	char *ign;
	char *clk_name;
	char *rate_str;
	struct clk *clk;
	unsigned long rate;
	int r;

	strncpy(cmd, last_cmd, sizeof(last_cmd));

	ign = strsep(&c, " ");
	clk_name = strsep(&c, " ");
	rate_str = strsep(&c, " ");
	r = kstrtoul(rate_str, 0, &rate);

	seq_printf(s, "clk_set_rate(%s, %lu): %d: ", clk_name, rate, r);

	clk = __clk_lookup(clk_name);
	if (IS_ERR_OR_NULL(clk)) {
		seq_printf(s, "clk_get(): 0x%p\n", clk);
		return PTR_ERR(clk);
	}

	r = clk_set_rate(clk, rate);
	seq_printf(s, "%d\n", r);

	return r;
}

void *reg_from_str(const char *str)
{
	if (sizeof(void *) == sizeof(unsigned long)) {
		unsigned long v;

		if (kstrtoul(str, 0, &v) == 0)
			return (void *)((uintptr_t)v);
	} else if (sizeof(void *) == sizeof(unsigned long long)) {
		unsigned long long v;

		if (kstrtoull(str, 0, &v) == 0)
			return (void *)((uintptr_t)v);
	} else {
		unsigned long long v;

		clk_warn("unexpected pointer size: sizeof(void *): %zu\n",
			sizeof(void *));

		if (kstrtoull(str, 0, &v) == 0)
			return (void *)((uintptr_t)v);
	}

	clk_warn("%s(): parsing error: %s\n", __func__, str);

	return NULL;
}

static int parse_reg_val_from_cmd(void __iomem **preg, unsigned long *pval)
{
	char cmd[sizeof(last_cmd) + 1];
	char *c = cmd;
	char *ign;
	char *reg_str;
	char *val_str;
	int r = 0;

	strncpy(cmd, last_cmd, sizeof(last_cmd));

	ign = strsep(&c, " ");
	reg_str = strsep(&c, " ");
	val_str = strsep(&c, " ");

	if (preg)
		*preg = reg_from_str(reg_str);

	if (pval)
		r = kstrtoul(val_str, 0, pval);

	return r;
}

static int clkdbg_reg_read(struct seq_file *s, void *v)
{
	void __iomem *reg;
	unsigned long val;

	parse_reg_val_from_cmd(&reg, NULL);
	seq_printf(s, "readl(0x%p): ", reg);

	val = clk_readl(reg);
	seq_printf(s, "0x%08x\n", (u32)val);

	return 0;
}

static int clkdbg_reg_write(struct seq_file *s, void *v)
{
	void __iomem *reg;
	unsigned long val;

	parse_reg_val_from_cmd(&reg, &val);
	seq_printf(s, "writel(0x%p, 0x%08x): ", reg, (u32)val);

	clk_writel(reg, val);
	val = clk_readl(reg);
	seq_printf(s, "0x%08x\n", (u32)val);

	return 0;
}

static int clkdbg_reg_set(struct seq_file *s, void *v)
{
	void __iomem *reg;
	unsigned long val;

	parse_reg_val_from_cmd(&reg, &val);
	seq_printf(s, "writel(0x%p, 0x%08x): ", reg, (u32)val);

	clk_setl(reg, val);
	val = clk_readl(reg);
	seq_printf(s, "0x%08x\n", (u32)val);

	return 0;
}

static int clkdbg_reg_clr(struct seq_file *s, void *v)
{
	void __iomem *reg;
	unsigned long val;

	parse_reg_val_from_cmd(&reg, &val);
	seq_printf(s, "writel(0x%p, 0x%08x): ", reg, (u32)val);

	clk_clrl(reg, val);
	val = clk_readl(reg);
	seq_printf(s, "0x%08x\n", (u32)val);

	return 0;
}

static int clkdbg_show(struct seq_file *s, void *v)
{
	static const struct {
		const char	*cmd;
		int (*fn)(struct seq_file *, void *);
	} cmds[] = {
		/* {.cmd = "dump_regs",		.fn = seq_print_regs}, */
		{.cmd = "dump_state",		.fn = clkdbg_dump_state_all},
		/* {.cmd = "fmeter",		.fn = seq_print_fmeter_all}, */
		{.cmd = "prepare_enable",	.fn = clkdbg_prepare_enable},
		{.cmd = "disable_unprepare",	.fn = clkdbg_disable_unprepare},
		{.cmd = "set_parent",		.fn = clkdbg_set_parent},
		{.cmd = "set_rate",		.fn = clkdbg_set_rate},
		{.cmd = "reg_read",		.fn = clkdbg_reg_read},
		{.cmd = "reg_write",		.fn = clkdbg_reg_write},
		{.cmd = "reg_set",		.fn = clkdbg_reg_set},
		{.cmd = "reg_clr",		.fn = clkdbg_reg_clr},
	};

	int i;
	char cmd[sizeof(last_cmd) + 1];

	pr_debug("last_cmd: %s\n", last_cmd);

	strncpy(cmd, last_cmd, sizeof(last_cmd));

	for (i = 0; i < ARRAY_SIZE(cmds); i++) {
		char *c = cmd;
		char *token = strsep(&c, " ");

		if (strcmp(cmds[i].cmd, token) == 0)
			return cmds[i].fn(s, v);
	}

	return 0;
}

static int clkdbg_open(struct inode *inode, struct file *file)
{
	return single_open(file, clkdbg_show, NULL);
}

static ssize_t clkdbg_write(
		struct file *file,
		const char __user *buffer,
		size_t count,
		loff_t *data)
{
	char desc[sizeof(last_cmd) - 1];
	int len = 0;

	pr_debug("count: %zu\n", count);
	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';
	strncpy(last_cmd, desc, sizeof(last_cmd));
	if (last_cmd[len - 1] == '\n')
		last_cmd[len - 1] = 0;

	return count;
}

static const struct file_operations clkdbg_fops = {
	.owner		= THIS_MODULE,
	.open		= clkdbg_open,
	.read		= seq_read,
	.write		= clkdbg_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};

int __init mt_clkdbg_init(void)
{
	/* init_iomap(); */

	/* print_regs(); */
	/* print_fmeter_all(); */

	return 0;
}
arch_initcall(mt_clkdbg_init);

int __init mt_clkdbg_debug_init(void)
{
	struct proc_dir_entry *entry;

	entry = proc_create("clkdbg", 0, 0, &clkdbg_fops);
	if (!entry)
		return -ENOMEM;

	return 0;
}
module_init(mt_clkdbg_debug_init);

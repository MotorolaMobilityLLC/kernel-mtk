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

#define LOG_TAG "dump"

#include "disp_drv_platform.h"
#include "ddp_irq.h"
#include "ddp_reg.h"
#include "disp_log.h"
#include "ddp_dump.h"
#include "ddp_ovl.h"
#include "ddp_wdma.h"
#include "ddp_wdma_ex.h"
#include "ddp_rdma.h"
#include "ddp_rdma_ex.h"

#ifdef CONFIG_MTK_CLKMGR
#include <mach/mt_clkmgr.h>
#endif

#include "disp_assert_layer.h"

static char *ddp_signal_0(int bit)
{
	switch (bit) {
	case 31:
		return "dpi0_sel_mm_dpi0          ";
	case 30:
		return "dis0_sel_mm_dsi0          ";
	case 29:
		return "rdma1_sout1_mm_dpi0_sin2  ";
	case 28:
		return "rdma1_sout0_mm_dsi0_sin2  ";
	case 27:
		return "rdma1_mm_rdma1_sout       ";
	case 26:
		return "ovl1_mout2_mm_ovl0        ";
	case 25:
		return "ovl1_mout1_mm_wdma1       ";
	case 24:
		return "ovl1_mout0_mm_rdma1       ";
	case 23:
		return "ovl1_mm_ovl1_mout         ";
	case 22:
		return "wdma0_sel_mm_wdma0        ";
	case 21:
		return "ufoe_mout2_mm_wdma0_sin2  ";
	case 20:
		return "ufoe_mout1_mm_dpi0_sin0   ";
	case 19:
		return "ufoe_mout0_mm_dsi0_sin0   ";
	case 18:
		return "ufoe_mm_ufoe_mout         ";
	case 17:
		return "ufoe_sel_mm_ufoe          ";
	case 16:
		return "rdma0_sout3_mm_dpi0_sin1  ";
	case 15:
		return "rdma0_sout2_mm_dsi0_sin1  ";
	case 14:
		return "rdma0_sout1_mm_color_sin0 ";
	case 13:
		return "rdma0_sout0_mm_ufoe_sin0  ";
	case 12:
		return "rdma0_mm_rdma0_sout       ";
	case 11:
		return "dither_mout2_mm_wdma0_sin1";
	case 10:
		return "dither_mout1_mm_ufoe_sin1 ";
	case 9:
		return "dither_mout0_mm_rdma0     ";
	case 8:
		return "dither_mm_dither_mout     ";
	case 7:
		return "gamma_mm_dither           ";
	case 6:
		return "aal_mm_gamma              ";
	case 5:
		return "ccorr_mm_aal              ";
	case 4:
		return "color_mm_ccorr            ";
	case 3:
		return "color_sel_mm_color        ";
	case 2:
		return "ovl0_mout1_mm_wdma0_sin0  ";
	case 1:
		return "ovl0_mout0_mm_color_sin1  ";
	case 0:
		return "ovl0_mm_ovl0_mout         ";
	default:
		DISPERR("ddp_signal_0, unknown bit=%d\n", bit);
		return "unknown";
	}
}

/* 1 means SMI dose not grant, maybe SMI hang */
static int ddp_greq_check(int reg_val)
{
	if (reg_val & 0x3f)
		DISPMSG("smi greq not grant module: ");
	else
		return 0;

	if (reg_val & 0x1)
		DISPMSG("disp_wdma1, ");
	if (reg_val & 0x1)
		DISPMSG("disp_rdma1, ");
	if (reg_val & 0x1)
		DISPMSG("disp_ovl1, ");
	if (reg_val & 0x1)
		DISPMSG("disp_wdma0, ");
	if (reg_val & 0x1)
		DISPMSG("disp_rdma0, ");
	if (reg_val & 0x1)
		DISPMSG("disp_ovl0, ");
	DISPMSG("\n");

	return 0;
}

static char *ddp_get_mutex_module_name(unsigned int bit)
{
	switch (bit) {
	case 6:
		return "ovl0";
	case 7:
		return "ovl1";
	case 8:
		return "rdma0";
	case 9:
		return "rdma1";
	case 10:
		return "wdma0";
	case 11:
		return "color0";
	case 12:
		return "ccorr";
	case 13:
		return "aal";
	case 14:
		return "gamma";
	case 15:
		return "dither";
	case 16:
		return "ufoe";
	case 17:
		return "pwm0";
	case 18:
		return "wdma1";
	default:
		return "mutex-unknown";
	}
}

char *ddp_get_fmt_name(DISP_MODULE_ENUM module, unsigned int fmt)
{
	if (module == DISP_MODULE_WDMA0 || module == DISP_MODULE_WDMA1) {
		switch (fmt) {
		case 0:
			return "rgb565";
		case 1:
			return "rgb888";
		case 2:
			return "rgba8888";
		case 3:
			return "argb8888";
		case 4:
			return "uyvy";
		case 5:
			return "yuy2";
		case 7:
			return "y-only";
		case 8:
			return "iyuv";
		case 12:
			return "nv12";
		default:
			DISPDMP("ddp_get_fmt_name, unknown fmt=%d, module=%d\n", fmt, module);
			return "unknown";
		}
	} else if (module == DISP_MODULE_OVL0 || module == DISP_MODULE_OVL1) {
		switch (fmt) {
		case 0:
			return "rgb565";
		case 1:
			return "rgb888";
		case 2:
			return "rgba8888";
		case 3:
			return "argb8888";
		case 4:
			return "uyvy";
		case 5:
			return "yuyv";
		default:
			DISPDMP("ddp_get_fmt_name, unknown fmt=%d, module=%d\n", fmt, module);
			return "unknown";
		}
	} else if (module == DISP_MODULE_RDMA0 || module == DISP_MODULE_RDMA1 || module == DISP_MODULE_RDMA2) {
		/* todo: confirm with designers */
		switch (fmt) {
		case 0:
			return "rgb565";
		case 1:
			return "rgb888";
		case 2:
			return "rgba8888";
		case 3:
			return "argb8888";
		case 4:
			return "uyvy";
		case 5:
			return "yuyv";
		default:
			DISPDMP("ddp_get_fmt_name, unknown fmt=%d, module=%d\n", fmt, module);
			return "unknown";
		}
	} else if (module == DISP_MODULE_MUTEX) {
		switch (fmt) {
		case 0:
			return "single";
		case 1:
			return "dsi0_vdo";
		case 2:
			return "dsi1_vdo";
		case 3:
			return "dpi";
		default:
			DISPDMP("ddp_get_fmt_name, unknown fmt=%d, module=%d\n", fmt, module);
			return "unknown";
		}
	} else {
		DISPDMP("ddp_get_fmt_name, unknown module=%d\n", module);
	}

	return "unknown";
}

static char *ddp_clock_0(int bit)
{
	switch (bit) {
	case 0:
		return "smi_common, ";
	case 1:
		return "smi_larb0, ";
	case 10:
		return "ovl0, ";
	case 11:
		return "ovl1, ";
	case 12:
		return "rdma0, ";
	case 13:
		return "rdma1, ";
	case 14:
		return "wdma0, ";
	case 15:
		return "color, ";
	case 16:
		return "ccorr, ";
	case 17:
		return "aal, ";
	case 18:
		return "gamma, ";
	case 19:
		return "dither, ";
	case 20:
		return "ufoe, ";
	case 21:
		return "larb4_mm, ";
	case 22:
		return "larb4_mjc, ";
	case 23:
		return "wdma1, ";
	default:
		return "";
	}
}

static char *ddp_clock_1(int bit)
{
	switch (bit) {
	case 0:
		return "disp_pwm_mm, ";
	case 1:
		return "disp_pwm_26m, ";
	case 2:
		return "dsi_engine, ";
	case 3:
		return "dsi_digital, ";
	case 4:
		return "dpi_pixel, ";
	case 5:
		return "dpi_engine, ";
	default:
		return "";
	}
}

static void mutex_dump_reg(void)
{
	DISPDMP("==DISP MUTEX REGS==\n");
	DISPDMP("(0x000)M_INTEN   =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX_INTEN));
	DISPDMP("(0x004)M_INTSTA  =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX_INTSTA));
	DISPDMP("(0x008)M_HW_DCM  =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX_HW_DCM));
	DISPDMP("(0x020)M0_EN     =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX0_EN));
	DISPDMP("(0x028)M0_RST    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX0_RST));
	DISPDMP("(0x02c)M0_MOD    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX0_MOD));
	DISPDMP("(0x030)M0_SOF    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX0_SOF));
	DISPDMP("(0x040)M1_EN     =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX1_EN));
	DISPDMP("(0x048)M1_RST    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX1_RST));
	DISPDMP("(0x04c)M1_MOD    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX1_MOD));
	DISPDMP("(0x050)M1_SOF    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX1_SOF));
	DISPDMP("(0x060)M2_EN     =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX2_EN));
	DISPDMP("(0x068)M2_RST    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX2_RST));
	DISPDMP("(0x06c)M2_MOD    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX2_MOD));
	DISPDMP("(0x070)M2_SOF    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX2_SOF));
	DISPDMP("(0x080)M3_EN     =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX3_EN));
	DISPDMP("(0x088)M3_RST    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX3_RST));
	DISPDMP("(0x08c)M3_MOD    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX3_MOD));
	DISPDMP("(0x090)M3_SOF    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX3_SOF));
	DISPDMP("(0x0a0)M4_EN     =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX4_EN));
	DISPDMP("(0x0a8)M4_RST    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX4_RST));
	DISPDMP("(0x0ac)M4_MOD    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX4_MOD));
	DISPDMP("(0x0b0)M4_SOF    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX4_SOF));
	DISPDMP("(0x0c0)M5_EN     =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX5_EN));
	DISPDMP("(0x0c8)M5_RST    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX5_RST));
	DISPDMP("(0x0cc)M5_MOD    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX5_MOD));
	DISPDMP("(0x0d0)M5_SOF    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX5_SOF));
	DISPDMP("(0x200)DEBUG_OUT_SEL =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DEBUG_OUT_SEL));
}

static void mutex_dump_analysis(void)
{
	int i = 0;
	int j = 0;
	char mutex_module[512] = { '\0' };
	char *p = NULL;
	int len = 0;

	DISPDMP("==DISP Mutex Analysis==\n");
	for (i = 0; i < 5; i++) {
		p = mutex_module;
		len = 0;
		if (DISP_REG_GET(DISP_REG_CONFIG_MUTEX_MOD(i)) != 0 &&
		    ((DISP_REG_GET(DISP_REG_CONFIG_MUTEX_EN(i) + 0x20 * i) == 1 &&
		      DISP_REG_GET(DISP_REG_CONFIG_MUTEX_SOF(i) + 0x20 * i) != SOF_SINGLE) ||
		     DISP_REG_GET(DISP_REG_CONFIG_MUTEX_SOF(i) + 0x20 * i) == SOF_SINGLE)) {
			len =
			    sprintf(p, "MUTEX%d :mode=%s,module=(", i,
				    ddp_get_fmt_name(DISP_MODULE_MUTEX,
						     DISP_REG_GET(DISP_REG_CONFIG_MUTEX_SOF(i))));
			p += len;
			for (j = 6; j <= 18; j++) {
				if ((DISP_REG_GET(DISP_REG_CONFIG_MUTEX_MOD(i)) >> j) & 0x1) {
					len = sprintf(p, "%s,", ddp_get_mutex_module_name(j));
					p += len;
				}
			}
			DISPDMP("%s)\n", mutex_module);
			DISPDMP("irq cnt: start=%d, end=%d\n", mutex_start_irq_cnt,
				mutex_done_irq_cnt);
		}
	}
}

static void mmsys_config_dump_reg(void)
{
	DISPDMP("== DISP Config  ==\n");
	DISPDMP("(0x0 )MMSYS_INTEN      =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_INTEN));
	DISPDMP("(0x4 )MMSYS_INTSTA     =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_INTSTA));
	DISPDMP("(0x30)OVL0_MOUT_EN     =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_OVL0_MOUT_EN));
#if defined(MTK_FB_OVL1_SUPPORT)
	DISPDMP("(0x34)OVL1_MOUT_EN     =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_OVL1_MOUT_EN));
#endif
	DISPDMP("(0x38)DITHER_MOUT_EN   =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_DITHER_MOUT_EN));
	DISPDMP("(0x3C)UFOE_MOUT_EN     =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_UFOE_MOUT_EN));
	DISPDMP("(0x40)MMSYS_MOUT_RST   =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_MOUT_RST));
	DISPDMP("(0x58)COLOR0_SIN       =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_COLOR0_SEL_IN));
	DISPDMP("(0x5C)WDMA0_SIN        =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_WDMA0_SEL_IN));
	DISPDMP("(0x60)UFOE_SIN         =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_UFOE_SEL_IN));
	DISPDMP("(0x64)DSI0_SIN         =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DSI0_SEL_IN));
	DISPDMP("(0x68)DPI0_SIN         =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DPI0_SEL_IN));
	DISPDMP("(0x6C)RDMA0_SOUT_SIN   =0x%x\n",
	       DISP_REG_GET(DISP_REG_CONFIG_DISP_RDMA0_SOUT_SEL_IN));
	DISPDMP("(0x70)RDMA1_SOUT_SIN   =0x%x\n",
	       DISP_REG_GET(DISP_REG_CONFIG_DISP_RDMA1_SOUT_SEL_IN));
	DISPDMP("(0x0F0)MM_MISC         =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_MISC));
	DISPDMP("(0x100)MM_CG_CON0      =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON0));
	DISPDMP("(0x110)MM_CG_CON1      =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON1));
	DISPDMP("(0x120)MM_HW_DCM_DIS0  =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_HW_DCM_DIS0));
	DISPDMP("(0x140)MM_SW0_RST_B    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_SW0_RST_B));
	DISPDMP("(0x144)MM_SW1_RST_B    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_SW1_RST_B));
	DISPDMP("(0x150)MM_LCM_RST_B    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_LCM_RST_B));
	DISPDMP("(0x880)MM_DBG_OUT_SEL  =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_DEBUG_OUT_SEL));
	DISPDMP("(0x890)MM_DUMMY        =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_DUMMY));
	DISPDMP("(0x8a0)DISP_VALID_0    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_DL_VALID_0));
	DISPDMP("(0x8a8)DISP_READY_0    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_DL_READY_0));
	DISPDMP("(0xc08)C08             =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_C08));
	DISPDMP("(0x40 )C09             =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_C09));
	DISPDMP("(0x44 )C10             =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_C10));
}

/* ------ clock:
Before power on mmsys:
CLK_CFG_0_CLR (address is 0x10000048) = 0x80000000 (bit 31).
Before using DISP_PWM0 or DISP_PWM1:
CLK_CFG_1_CLR(address is 0x10000058)=0x80 (bit 7).
Before using DPI pixel clock:
CLK_CFG_6_CLR(address is 0x100000A8)=0x80 (bit 7).

Only need to enable the corresponding bits of MMSYS_CG_CON0 and MMSYS_CG_CON1 for the modules:
smi_common, larb0, mdp_crop, fake_eng, mutex_32k, pwm0, pwm1, dsi0, dsi1, dpi.
Other bits could keep 1. Suggest to keep smi_common and larb0 always clock on.

--------valid & ready
example:
ovl0 -> ovl0_mout_ready=1 means engines after ovl_mout are ready for receiving data
	ovl0_mout_ready=0 means ovl0_mout can not receive data, maybe ovl0_mout or after engines config error
ovl0 -> ovl0_mout_valid=1 means engines before ovl0_mout is OK,
	ovl0_mout_valid=0 means ovl can not transfer data to ovl0_mout, means ovl0 or before engines are not ready.
*/

static void mmsys_config_dump_analysis(void)
{
	unsigned int i = 0;
	unsigned int reg = 0;
	char clock_on[512] = { '\0' };
	char *pos = NULL;
	int len = 0;
	unsigned int valid0, valid1, ready0, ready1, greq;

	DISPDMP("==DISP MMSYS_CONFIG ANALYSIS==\n");
	DISPDMP("[ddp] mmsys_clock=0x%x\n", DISP_REG_GET(DISP_REG_CLK_CFG_0_MM_CLK));
#ifdef CONFIG_MTK_CLKMGR
	DISPDMP("larb0_force_on=%d, smi_common_force_on=%d\n",
		 clk_is_force_on(MT_CG_DISP0_SMI_LARB0), clk_is_force_on(MT_CG_DISP0_SMI_COMMON));
#endif

	if ((DISP_REG_GET(DISP_REG_CLK_CFG_0_MM_CLK) >> 31) & 0x1) {
		DISPERR("mmsys clock abnormal!!\n");
		return;
	}

	DISPDMP("mmsys clock=0x%x, CG_CON0=0x%x, CG_CON1=0x%x\n",
		DISP_REG_GET(DISP_REG_CLK_CFG_0_MM_CLK),
		DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON0),
		DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON1));
#if 0
	DISPDMP("PLL clock=0x%x\n", DISP_REG_GET(DISP_REG_VENCPLL_CON0));
	if (!(DISP_REG_GET(DISP_REG_VENCPLL_CON0) & 0x1))
		DISPERR("PLL clock abnormal!!\n");

#endif
	reg = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON0);

	for (i = 0; i <= 1; i++) {
		if ((reg & (1 << i)) == 0)
			strncat(clock_on, ddp_clock_0(i), sizeof(clock_on) - strlen(clock_on) - 1);
	}
	for (i = 10; i <= 31; i++) {
		if ((reg & (1 << i)) == 0)
			strncat(clock_on, ddp_clock_0(i), sizeof(clock_on) - strlen(clock_on) - 1);
	}
	reg = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON1);
	for (i = 0; i <= 5; i++) {
		if ((reg & (1 << i)) == 0)
			strncat(clock_on, ddp_clock_1(i), sizeof(clock_on) - strlen(clock_on) - 1);
	}
	DISPDMP("clock on modules:%s\n", clock_on);

	DISPDMP("clock_ef setting:%u,%u\n",
		DISP_REG_GET(DISP_REG_CONFIG_C09), DISP_REG_GET(DISP_REG_CONFIG_C10));
#if 0
	DISPDMP("clock_mm setting:%u\n", DISP_REG_GET(DISP_REG_CONFIG_C11));
	if ((DISP_REG_GET(DISP_REG_CONFIG_C11) & 0xff000000) != 0xff000000) {
		DISPDMP("error, MM clock bit 24~bit31 should be 1, but real value=0x%x",
			DISP_REG_GET(DISP_REG_CONFIG_C11));
	}
#endif

	if (((DISP_REG_GET(DISP_REG_CONFIG_C09) >> 7) & 0x7) == 5 &&
	    ((DISP_REG_GET(DISP_REG_RDMA_SIZE_CON_0) & 0xfff) > 1200 ||
	     (DISP_REG_GET(DISP_REG_RDMA_SIZE_CON_1) & 0xfffff) > 1920)) {
		DISPERR("check clock1 setting error:(120,192),(%d, %d)\n",
		       DISP_REG_GET(DISP_REG_RDMA_SIZE_CON_0) & 0xfff,
		       DISP_REG_GET(DISP_REG_RDMA_SIZE_CON_1) & 0xfffff);
	} else if (((DISP_REG_GET(DISP_REG_CONFIG_C09) >> 8) & 0x3) == 3 &&
		   ((DISP_REG_GET(DISP_REG_RDMA_SIZE_CON_0) & 0xfff) > 800 ||
		    (DISP_REG_GET(DISP_REG_RDMA_SIZE_CON_1) & 0xfffff) > 1280)) {
		DISPERR("check clock1 setting error:(80,128),(%d, %d)\n",
		       DISP_REG_GET(DISP_REG_RDMA_SIZE_CON_0) & 0xfff,
		       DISP_REG_GET(DISP_REG_RDMA_SIZE_CON_1) & 0xfffff);
	}
	if (((DISP_REG_GET(DISP_REG_CONFIG_C10) >> 7) & 0x7) == 5 &&
	    ((DISP_REG_GET(DISP_REG_OVL_ROI_SIZE + DISP_OVL_INDEX_OFFSET * 1) & 0xfff) > 1200 ||
	     ((DISP_REG_GET(DISP_REG_OVL_ROI_SIZE + DISP_OVL_INDEX_OFFSET * 1) >> 16) & 0xfff) > 1920)) {
		DISPERR("check clock2 setting error:(120,192),(%d, %d)\n",
		       DISP_REG_GET(DISP_REG_OVL_ROI_SIZE + DISP_OVL_INDEX_OFFSET * 1) & 0xfff,
		       (DISP_REG_GET(DISP_REG_OVL_ROI_SIZE + DISP_OVL_INDEX_OFFSET * 1) >> 16) &
		       0xfff);
	} else if (((DISP_REG_GET(DISP_REG_CONFIG_C10) >> 8) & 0x3) == 3 &&
		   ((DISP_REG_GET(DISP_REG_OVL_ROI_SIZE + DISP_OVL_INDEX_OFFSET * 1) & 0xfff) > 800 ||
		   ((DISP_REG_GET(DISP_REG_OVL_ROI_SIZE + DISP_OVL_INDEX_OFFSET * 1) >> 16) & 0xfff) > 1280)) {
		DISPERR("check clock2 setting error:(80,128),(%d, %d)\n",
		       DISP_REG_GET(DISP_REG_OVL_ROI_SIZE + DISP_OVL_INDEX_OFFSET * 1) & 0xfff,
		       (DISP_REG_GET(DISP_REG_OVL_ROI_SIZE + DISP_OVL_INDEX_OFFSET * 1) >> 16) & 0xfff);
	}


	valid0 = DISP_REG_GET(DISPSYS_CONFIG_BASE + 0x8a0);
	valid1 = DISP_REG_GET(DISPSYS_CONFIG_BASE + 0x8a4);
	ready0 = DISP_REG_GET(DISPSYS_CONFIG_BASE + 0x8a8);
	ready1 = DISP_REG_GET(DISPSYS_CONFIG_BASE + 0x8ac);
	greq = DISP_REG_GET(DISPSYS_CONFIG_BASE + 0x8d0);
	for (i = 0; i < 5; i++) {
		DISPDMP("v0=0x%x, v1=0x%x, r0=0x%x, r1=0x%x, greq=0%x\n",
			DISP_REG_GET(DISPSYS_CONFIG_BASE + 0x8a0),
			DISP_REG_GET(DISPSYS_CONFIG_BASE + 0x8a4),
			DISP_REG_GET(DISPSYS_CONFIG_BASE + 0x8a8),
			DISP_REG_GET(DISPSYS_CONFIG_BASE + 0x8ac),
			DISP_REG_GET(DISPSYS_CONFIG_BASE + 0x8d0));
	}
	for (i = 0; i < 32; i++) {
		pos = clock_on;
		if ((valid0 & (1 << i)) == 0)
			len = sprintf(pos, "%s:-", ddp_signal_0(i));
		else
			len = sprintf(pos, "%s:V,", ddp_signal_0(i));

		pos += len;
		if ((ready0 & (1 << i)) == 0)
			len = sprintf(pos, "-");
		else
			len = sprintf(pos, "R");

		DISPDMP("%s\n", clock_on);
	}

	ddp_greq_check(greq);

	/* AEE status dump */
	{
		if (isAEEEnabled == 0) {
			DISPDMP("isAEEEnabled=%d\n", isAEEEnabled);
		} else {
			DISPDMP("isAEEEnabled=%d, O0L3=0x%x, AEE=0x%lx\n", isAEEEnabled,
				DISP_REG_GET(DISP_REG_OVL_L3_ADDR), get_Assert_Layer_PA());
			if (DISP_REG_GET(DISP_REG_OVL_L3_ADDR) != get_Assert_Layer_PA())
				DISPDMP("error: AEE enabled but O-L3 Addr wrong!\n");
		}
	}
}

static void gamma_dump_reg(void)
{
	DISPDMP("==DISP GAMMA REGS==\n");
	DISPDMP("(0x000)GA_EN        =0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_EN));
	DISPDMP("(0x004)GA_RESET     =0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_RESET));
	DISPDMP("(0x008)GA_INTEN     =0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_INTEN));
	DISPDMP("(0x00c)GA_INTSTA    =0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_INTSTA));
	DISPDMP("(0x010)GA_STATUS    =0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_STATUS));
	DISPDMP("(0x020)GA_CFG       =0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_CFG));
	DISPDMP("(0x024)GA_IN_COUNT  =0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_INPUT_COUNT));
	DISPDMP("(0x028)GA_OUT_COUNT =0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_OUTPUT_COUNT));
	DISPDMP("(0x02c)GA_CHKSUM    =0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_CHKSUM));
	DISPDMP("(0x030)GA_SIZE      =0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_SIZE));
	DISPDMP("(0x0c0)GA_DUMMY_REG =0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_DUMMY_REG));
	DISPDMP("(0x800)GA_LUT       =0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_LUT));
}

static void gamma_dump_analysis(void)
{
	DISPDMP("==DISP GAMMA ANALYSIS==\n");
	DISPDMP("gamma: en=%d, w=%d, h=%d, in_p_cnt=%d, in_l_cnt=%d, out_p_cnt=%d, out_l_cnt=%d\n",
		DISP_REG_GET(DISP_REG_GAMMA_EN),
		(DISP_REG_GET(DISP_REG_GAMMA_SIZE) >> 16) & 0x1fff,
		DISP_REG_GET(DISP_REG_GAMMA_SIZE) & 0x1fff,
		DISP_REG_GET(DISP_REG_GAMMA_INPUT_COUNT) & 0x1fff,
		(DISP_REG_GET(DISP_REG_GAMMA_INPUT_COUNT) >> 16) & 0x1fff,
		DISP_REG_GET(DISP_REG_GAMMA_OUTPUT_COUNT) & 0x1fff,
		(DISP_REG_GET(DISP_REG_GAMMA_OUTPUT_COUNT) >> 16) & 0x1fff);
}

static void merge_dump_reg(void)
{
	DISPDMP("==DISP MERGE REGS==\n");
	DISPDMP("(0x000)MERGE_EN       =0x%x\n", DISP_REG_GET(DISP_REG_MERGE_ENABLE));
	DISPDMP("(0x004)MERGE_SW_RESET =0x%x\n", DISP_REG_GET(DISP_REG_MERGE_SW_RESET));
	DISPDMP("(0x008)MERGE_DEBUG    =0x%x\n", DISP_REG_GET(DISP_REG_MERGE_DEBUG));
}

static void merge_dump_analysis(void)
{
	DISPDMP("==DISP MERGE ANALYSIS==\n");
	DISPDMP("merge: en=%d, debug=0x%x\n", DISP_REG_GET(DISP_REG_MERGE_ENABLE),
		DISP_REG_GET(DISP_REG_MERGE_DEBUG));
}

static void split_dump_reg(DISP_MODULE_ENUM module)
{
	DISPDMP("error: disp_split dose not exist! module=%d\n", module);
}

static void split_dump_analysis(DISP_MODULE_ENUM module)
{
	DISPDMP("error: disp_split dose not exist! module=%d\n", module);
}

static void color_dump_reg(DISP_MODULE_ENUM module)
{
	int index = 0;

	if (DISP_MODULE_COLOR0 == module) {
		index = 0;
	} else if (DISP_MODULE_COLOR1 == module) {
		DISPDMP("error: DISP COLOR%d dose not exist!\n", index);
		return;
	}
	DISPDMP("==DISP COLOR%d REGS==\n", index);
	DISPDMP("(0x400)COLOR_CFG_MAIN   =0x%x\n", DISP_REG_GET(DISP_COLOR_CFG_MAIN));
	DISPDMP("(0x404)COLOR_PXL_CNT_MAIN   =0x%x\n", DISP_REG_GET(DISP_COLOR_PXL_CNT_MAIN));
	DISPDMP("(0x408)COLOR_LINE_CNT_MAIN   =0x%x\n", DISP_REG_GET(DISP_COLOR_LINE_CNT_MAIN));
	DISPDMP("(0xc00)COLOR_START      =0x%x\n", DISP_REG_GET(DISP_COLOR_START));
	DISPDMP("(0xc50)COLOR_INTER_IP_W =0x%x\n", DISP_REG_GET(DISP_COLOR_INTERNAL_IP_WIDTH));
	DISPDMP("(0xc54)COLOR_INTER_IP_H =0x%x\n", DISP_REG_GET(DISP_COLOR_INTERNAL_IP_HEIGHT));
}

static void color_dump_analysis(DISP_MODULE_ENUM module)
{
	int index = 0;

	if (DISP_MODULE_COLOR0 == module) {
		index = 0;
	} else if (DISP_MODULE_COLOR1 == module) {
		DISPDMP("error: DISP COLOR%d dose not exist!\n", index);
		return;
	}
	DISPDMP("==DISP COLOR%d ANALYSIS==\n", index);
	DISPDMP("color%d: bypass=%d, w=%d, h=%d, pixel_cnt=%d, line_cnt=%d,\n",
		index,
		(DISP_REG_GET(DISP_COLOR_CFG_MAIN) >> 7) & 0x1,
		DISP_REG_GET(DISP_COLOR_INTERNAL_IP_WIDTH),
		DISP_REG_GET(DISP_COLOR_INTERNAL_IP_HEIGHT),
		DISP_REG_GET(DISP_COLOR_PXL_CNT_MAIN) & 0xffff,
		(DISP_REG_GET(DISP_COLOR_LINE_CNT_MAIN) >> 16) & 0x1fff);
}

static void aal_dump_reg(void)
{
	DISPDMP("==DISP AAL REGS==\n");
	DISPDMP("(0x000)AAL_EN           =0x%x\n", DISP_REG_GET(DISP_AAL_EN));
	DISPDMP("(0x008)AAL_INTEN        =0x%x\n", DISP_REG_GET(DISP_AAL_INTEN));
	DISPDMP("(0x00c)AAL_INTSTA       =0x%x\n", DISP_REG_GET(DISP_AAL_INTSTA));
	DISPDMP("(0x020)AAL_CFG          =0x%x\n", DISP_REG_GET(DISP_AAL_CFG));
	DISPDMP("(0x024)AAL_IN_CNT       =0x%x\n", DISP_REG_GET(DISP_AAL_IN_CNT));
	DISPDMP("(0x028)AAL_OUT_CNT      =0x%x\n", DISP_REG_GET(DISP_AAL_OUT_CNT));
	DISPDMP("(0x030)AAL_SIZE         =0x%x\n", DISP_REG_GET(DISP_AAL_SIZE));
	DISPDMP("(0x20c)AAL_CABC_00      =0x%x\n", DISP_REG_GET(DISP_AAL_CABC_00));
	DISPDMP("(0x214)AAL_CABC_02      =0x%x\n", DISP_REG_GET(DISP_AAL_CABC_02));
	DISPDMP("(0x20c)AAL_STATUS_00    =0x%x\n", DISP_REG_GET(DISP_AAL_STATUS_00));
	DISPDMP("(0x210)AAL_STATUS_01    =0x%x\n", DISP_REG_GET(DISP_AAL_STATUS_00 + 0x4));
	DISPDMP("(0x2a0)AAL_STATUS_31    =0x%x\n", DISP_REG_GET(DISP_AAL_STATUS_32 - 0x4));
	DISPDMP("(0x2a4)AAL_STATUS_32    =0x%x\n", DISP_REG_GET(DISP_AAL_STATUS_32));
	DISPDMP("(0x3b0)AAL_DRE_MAPPING_00     =0x%x\n", DISP_REG_GET(DISP_AAL_DRE_MAPPING_00));
}

static void aal_dump_analysis(void)
{
	DISPDMP("==DISP AAL ANALYSIS==\n");
	DISPDMP("aal: bypass=%d, relay=%d, en=%d, w=%d, h=%d, in(%d,%d),out(%d,%d)\n",
		DISP_REG_GET(DISP_AAL_EN) == 0x0,
		DISP_REG_GET(DISP_AAL_CFG) & 0x01,
		DISP_REG_GET(DISP_AAL_EN),
		(DISP_REG_GET(DISP_AAL_SIZE) >> 16) & 0x1fff,
		DISP_REG_GET(DISP_AAL_SIZE) & 0x1fff,
		DISP_REG_GET(DISP_AAL_IN_CNT) & 0x1fff,
		(DISP_REG_GET(DISP_AAL_IN_CNT) >> 16) & 0x1fff,
		DISP_REG_GET(DISP_AAL_OUT_CNT) & 0x1fff,
		(DISP_REG_GET(DISP_AAL_OUT_CNT) >> 16) & 0x1fff);
}

static void pwm_dump_reg(DISP_MODULE_ENUM module)
{
	int index = 0;
	unsigned long reg_base = 0;

	if (module == DISP_MODULE_PWM0) {
		index = 0;
		reg_base = DISPSYS_PWM0_BASE;
	} else {
		index = 1;
		reg_base = DISPSYS_PWM1_BASE;
	}
	DISPDMP("==DISP PWM%d REGS==\n", index);
	DISPDMP("(0x000)PWM_EN           =0x%x\n", DISP_REG_GET(reg_base + DISP_PWM_EN_OFF));
	DISPDMP("(0x008)PWM_CON_0        =0x%x\n", DISP_REG_GET(reg_base + DISP_PWM_CON_0_OFF));
	DISPDMP("(0x010)PWM_CON_1        =0x%x\n", DISP_REG_GET(reg_base + DISP_PWM_CON_1_OFF));
	DISPDMP("(0x028)PWM_DEBUG        =0x%x\n", DISP_REG_GET(reg_base + 0x28));
}

static void pwm_dump_analysis(DISP_MODULE_ENUM module)
{
	int index = 0;
	unsigned int reg_base = 0;

	if (module == DISP_MODULE_PWM0) {
		index = 0;
		reg_base = DISPSYS_PWM0_BASE;
	} else {
		index = 1;
		reg_base = DISPSYS_PWM1_BASE;
	}
	DISPDMP("==DISP PWM%d ANALYSIS==\n", index);
	DISPDMP("pwm clock=%d\n", (DISP_REG_GET(DISP_REG_CLK_CFG_1_CLR) >> 7) & 0x1);
}

static void od_dump_reg(void)
{
	DISPDMP("==DISP OD REGS==\n");
	DISPDMP("(00)EN           =0x%x\n", DISP_REG_GET(DISP_REG_OD_EN));
	DISPDMP("(04)RESET        =0x%x\n", DISP_REG_GET(DISP_REG_OD_RESET));
	DISPDMP("(08)INTEN        =0x%x\n", DISP_REG_GET(DISP_REG_OD_INTEN));
	DISPDMP("(0C)INTSTA       =0x%x\n", DISP_REG_GET(DISP_REG_OD_INTSTA));
	DISPDMP("(10)STATUS       =0x%x\n", DISP_REG_GET(DISP_REG_OD_STATUS));
	DISPDMP("(20)CFG          =0x%x\n", DISP_REG_GET(DISP_REG_OD_CFG));
	DISPDMP("(24)INPUT_COUNT =0x%x\n", DISP_REG_GET(DISP_REG_OD_INPUT_COUNT));
	DISPDMP("(28)OUTPUT_COUNT =0x%x\n", DISP_REG_GET(DISP_REG_OD_OUTPUT_COUNT));
	DISPDMP("(2C)CHKSUM       =0x%x\n", DISP_REG_GET(DISP_REG_OD_CHKSUM));
	DISPDMP("(30)SIZE         =0x%x\n", DISP_REG_GET(DISP_REG_OD_SIZE));
	DISPDMP("(40)HSYNC_WIDTH  =0x%x\n", DISP_REG_GET(DISP_REG_OD_HSYNC_WIDTH));
	DISPDMP("(44)VSYNC_WIDTH  =0x%x\n", DISP_REG_GET(DISP_REG_OD_VSYNC_WIDTH));
	DISPDMP("(48)MISC         =0x%x\n", DISP_REG_GET(DISP_REG_OD_MISC));
	DISPDMP("(C0)DUMMY_REG    =0x%x\n", DISP_REG_GET(DISP_REG_OD_DUMMY_REG));
}

static void od_dump_analysis(void)
{
	DISPDMP("==DISP OD ANALYSIS==\n");
	DISPDMP("od: w=%d, h=%d, bypass=%d\n",
		(DISP_REG_GET(DISP_REG_OD_SIZE) >> 16) & 0xffff,
		DISP_REG_GET(DISP_REG_OD_SIZE) & 0xffff, DISP_REG_GET(DISP_REG_OD_CFG) & 0x1);
}

static void ccorr_dump_reg(void)
{
	DISPDMP("==DISP CCORR REGS==\n");
	DISPDMP("(00)EN   =0x%x\n", DISP_REG_GET(DISP_REG_CCORR_EN));
	DISPDMP("(20)CFG  =0x%x\n", DISP_REG_GET(DISP_REG_CCORR_CFG));
	DISPDMP("(24)IN_CNT =0x%x\n", DISP_REG_GET(DISP_REG_CCORR_IN_CNT));
	DISPDMP("(28)OUT_CNT =0x%x\n", DISP_REG_GET(DISP_REG_CCORR_OUT_CNT));
	DISPDMP("(30)SIZE =0x%x\n", DISP_REG_GET(DISP_REG_CCORR_SIZE));

}

static void ccorr_dump_analyze(void)
{
	DISPDMP("ccorr: en=%d, config=%d, w=%d, h=%d, in_p_cnt=%d, in_l_cnt=%d, out_p_cnt=%d, out_l_cnt=%d\n",
		DISP_REG_GET(DISP_REG_CCORR_EN), DISP_REG_GET(DISP_REG_CCORR_CFG),
		(DISP_REG_GET(DISP_REG_CCORR_SIZE) >> 16) & 0x1fff,
		DISP_REG_GET(DISP_REG_CCORR_SIZE) & 0x1fff,
		DISP_REG_GET(DISP_REG_CCORR_IN_CNT) & 0x1fff,
		(DISP_REG_GET(DISP_REG_CCORR_IN_CNT) >> 16) & 0x1fff,
		DISP_REG_GET(DISP_REG_CCORR_IN_CNT) & 0x1fff,
		(DISP_REG_GET(DISP_REG_CCORR_IN_CNT) >> 16) & 0x1fff);
}

static void dither_dump_reg(void)
{
	DISPDMP("==DISP DITHER REGS==\n");
	DISPDMP("(00)EN   =0x%x\n", DISP_REG_GET(DISP_REG_DITHER_EN));
	DISPDMP("(20)CFG  =0x%x\n", DISP_REG_GET(DISP_REG_DITHER_CFG));
	DISPDMP("(24)IN_CNT =0x%x\n", DISP_REG_GET(DISP_REG_DITHER_IN_CNT));
	DISPDMP("(28)OUT_CNT =0x%x\n", DISP_REG_GET(DISP_REG_DITHER_IN_CNT));
	DISPDMP("(30)SIZE =0x%x\n", DISP_REG_GET(DISP_REG_DITHER_SIZE));
}

static void dither_dump_analyze(void)
{
	DISPDMP("dither: en=%d, config=%d, w=%d, h=%d, in_p_cnt=%d, in_l_cnt=%d, out_p_cnt=%d, out_l_cnt=%d\n",
		     DISP_REG_GET(DISPSYS_DITHER_BASE + 0x000), DISP_REG_GET(DISPSYS_DITHER_BASE + 0x020),
		     (DISP_REG_GET(DISP_REG_DITHER_SIZE) >> 16) & 0x1fff,
		     DISP_REG_GET(DISP_REG_DITHER_SIZE) & 0x1fff,
		     DISP_REG_GET(DISP_REG_DITHER_IN_CNT) & 0x1fff,
		     (DISP_REG_GET(DISP_REG_DITHER_IN_CNT) >> 16) & 0x1fff,
		     DISP_REG_GET(DISP_REG_DITHER_OUT_CNT) & 0x1fff,
		     (DISP_REG_GET(DISP_REG_DITHER_OUT_CNT) >> 16) & 0x1fff);
}

static void dsi_dump_reg(DISP_MODULE_ENUM module)
{
	int i = 0;

	if (DISP_MODULE_DSI0 == module) {
		DISPDMP("==DISP DSI0 REGS==\n");
		for (i = 0; i < 25 * 16; i += 16)
			DISPDMP("DSI0+%04x : 0x%08x  0x%08x  0x%08x  0x%08x\n", i,
				 INREG32(DISPSYS_DSI0_BASE + i),
				 INREG32(DISPSYS_DSI0_BASE + i + 0x4),
				 INREG32(DISPSYS_DSI0_BASE + i + 0x8),
				 INREG32(DISPSYS_DSI0_BASE + i + 0xc));

		DISPDMP("DSI0 CMDQ+0x200 : 0x%08x  0x%08x  0x%08x  0x%08x\n",
			 INREG32(DISPSYS_DSI0_BASE + 0x200),
			 INREG32(DISPSYS_DSI0_BASE + 0x200 + 0x4),
			 INREG32(DISPSYS_DSI0_BASE + 0x200 + 0x8),
			 INREG32(DISPSYS_DSI0_BASE + 0x200 + 0xc));

#ifndef CONFIG_FPGA_EARLY_PORTING
		DISPDMP("==DISP MIPI REGS==\n");
		for (i = 0; i < 10 * 16; i += 16)
			DISPDMP("MIPI+%04x : 0x%08x  0x%08x  0x%08x  0x%08x\n", i,
				 INREG32(MIPITX_BASE + i), INREG32(MIPITX_BASE + i + 0x4),
				 INREG32(MIPITX_BASE + i + 0x8), INREG32(MIPITX_BASE + i + 0xc));

#endif
	}
#if 0
	else if (DISP_MODULE_DSI1 == module) {
		DISPDMP("==DISP DSI1 REGS==\n");
		for (i = 0; i < 20 * 16; i += 16)
			DISPDMP("DSI1+%04x : 0x%08x  0x%08x  0x%08x  0x%08x\n", i,
				 INREG32(DISPSYS_DSI1_BASE + i),
				 INREG32(DISPSYS_DSI1_BASE + i + 0x4),
				 INREG32(DISPSYS_DSI1_BASE + i + 0x8),
				 INREG32(DISPSYS_DSI1_BASE + i + 0xc));

	} else {
		DISPDMP("==DISP DSIDUAL REGS==\n");
		for (i = 0; i < 20 * 16; i += 16)
			DISPDMP("DSI0+%04x : 0x%08x  0x%08x  0x%08x  0x%08x\n", i,
				 INREG32(DISPSYS_DSI0_BASE + i),
				 INREG32(DISPSYS_DSI0_BASE + i + 0x4),
				 INREG32(DISPSYS_DSI0_BASE + i + 0x8),
				 INREG32(DISPSYS_DSI0_BASE + i + 0xc));

		for (i = 0; i < 20 * 16; i += 16)
			DISPDMP("DSI1+%04x : 0x%08x  0x%08x  0x%08x  0x%08x\n", i,
				 INREG32(DISPSYS_DSI1_BASE + i),
				 INREG32(DISPSYS_DSI1_BASE + i + 0x4),
				 INREG32(DISPSYS_DSI1_BASE + i + 0x8),
				 INREG32(DISPSYS_DSI1_BASE + i + 0xc));
	}
#endif
}

static void dpi_dump_analysis(void)
{
	DISPDMP("==DISP DPI ANALYSIS==\n");
	DISPDMP("DPI clock=0x%x\n", DISP_REG_GET(DISP_REG_CLK_CFG_6_DPI));
#if 0
	if ((DISP_REG_GET(DISP_REG_VENCPLL_CON0) >> 7) & 0x1)
		DISPDMP("DPI clock abnormal!!\n");
#endif
	DISPDMP("DPI  clock_clear=%d\n", (DISP_REG_GET(DISP_REG_CLK_CFG_6_CLR) >> 7) & 0x1);
}

int ddp_dump_reg(DISP_MODULE_ENUM module)
{
	switch (module) {
	case DISP_MODULE_WDMA0:
	case DISP_MODULE_WDMA1:
		wdma_dump_reg(module);
		break;
	case DISP_MODULE_RDMA0:
	case DISP_MODULE_RDMA1:
	case DISP_MODULE_RDMA2:
		rdma_dump_reg(module);
		break;
	case DISP_MODULE_OVL0:
	case DISP_MODULE_OVL1:
		ovl_dump_reg(module);
		break;
	case DISP_MODULE_GAMMA:
		gamma_dump_reg();
		break;
	case DISP_MODULE_CONFIG:
		mmsys_config_dump_reg();
		break;
	case DISP_MODULE_MUTEX:
		mutex_dump_reg();
		break;
	case DISP_MODULE_MERGE:
		merge_dump_reg();
		break;
	case DISP_MODULE_SPLIT0:
	case DISP_MODULE_SPLIT1:
		split_dump_reg(module);
		break;
	case DISP_MODULE_COLOR0:
	case DISP_MODULE_COLOR1:
		color_dump_reg(module);
		break;
	case DISP_MODULE_AAL:
		aal_dump_reg();
		break;
	case DISP_MODULE_PWM0:
	case DISP_MODULE_PWM1:
		pwm_dump_reg(module);
		break;
	case DISP_MODULE_UFOE:
		break;
	case DISP_MODULE_OD:
		od_dump_reg();
		break;
	case DISP_MODULE_DSI0:
	case DISP_MODULE_DSI1:
		dsi_dump_reg(module);
		break;
	case DISP_MODULE_DPI:
		break;
	case DISP_MODULE_CCORR:
		ccorr_dump_reg();
		break;
	case DISP_MODULE_DITHER:
		dither_dump_reg();
		break;
	default:
		DISPDMP("DDP error, dump_reg unknown module=%d\n", module);
	}
	return 0;
}

int ddp_dump_analysis(DISP_MODULE_ENUM module)
{
	switch (module) {
	case DISP_MODULE_WDMA0:
	case DISP_MODULE_WDMA1:
		wdma_dump_analysis(module);
		break;
	case DISP_MODULE_RDMA0:
	case DISP_MODULE_RDMA1:
	case DISP_MODULE_RDMA2:
		rdma_dump_analysis(module);
		break;
	case DISP_MODULE_OVL0:
	case DISP_MODULE_OVL1:
		ovl_dump_analysis(module);
		break;
	case DISP_MODULE_GAMMA:
		gamma_dump_analysis();
		break;
	case DISP_MODULE_CONFIG:
		mmsys_config_dump_analysis();
		break;
	case DISP_MODULE_MUTEX:
		mutex_dump_analysis();
		break;
	case DISP_MODULE_MERGE:
		merge_dump_analysis();
		break;
	case DISP_MODULE_SPLIT0:
	case DISP_MODULE_SPLIT1:
		split_dump_analysis(module);
		break;
	case DISP_MODULE_COLOR0:
	case DISP_MODULE_COLOR1:
		color_dump_analysis(module);
		break;
	case DISP_MODULE_AAL:
		aal_dump_analysis();
		break;
	case DISP_MODULE_UFOE:
		break;
	case DISP_MODULE_OD:
		od_dump_analysis();
		break;
	case DISP_MODULE_PWM0:
	case DISP_MODULE_PWM1:
		pwm_dump_analysis(module);
		break;
	case DISP_MODULE_DSI0:
	case DISP_MODULE_DSI1:
	case DISP_MODULE_DSIDUAL:
		break;
	case DISP_MODULE_DPI:
		dpi_dump_analysis();
		break;
	case DISP_MODULE_CCORR:
		ccorr_dump_analyze();
		break;
	case DISP_MODULE_DITHER:
		dither_dump_analyze();
		break;
	default:
		DISPDMP("DDP error, dump_analysis unknown module=%d\n", module);
	}
	return 0;
}

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

#include "ddp_reg.h"
#include "ddp_log.h"
#include "ddp_dump.h"
#include "ddp_ovl.h"
#include "ddp_wdma.h"
#include "ddp_wdma_ex.h"
#include "ddp_rdma.h"
#include "ddp_rdma_ex.h"
#include "ddp_dsi.h"

static char *ddp_signal_0(int bit)
{
	switch (bit) {
	case 31:
		return "UFOE_MOUT-WDMA0_SEL";
	case 30:
		return "UFOE_MOUT-DPI0_SEL";
	case 29:
		return "UFOE_MOUT-DISP_SPLIT";
	case 28:
		return "UFOE_MOUT-DSI0_SEL";
	case 27:
		return "UFOE-UFOE_MOUT";
	case 26:
		return "UFOE_SEL-UFOE";
	case 25:
		return "PATH0_SOUT-DSC_SEL";
	case 24:
		return "PATH0_SOUT-UFOE_SEL";
	case 23:
		return "PATH0_SEL-PATH0_SOUT";
	case 22:
		return "RDMA0_SOUT-DPI0_SEL";
	case 21:
		return "RDMA0_SOUT-DSI1_SEL";
	case 20:
		return "RDMA0_SOUT-DSI0_SEL";
	case 19:
		return "RDMA0_SOUT-COLOR_SEL";
	case 18:
		return "RDMA0_SOUT-PATH0_SEL";
	case 17:
		return "RMDA0-RDMA0_SOUT";
	case 16:
		return "DITHER_MOUT-WDMA0_SEL";
	case 15:
		return "DITHER_MOUT-PATH0_SEL";
	case 14:
		return "DITHER_MOUT-RMDA0";
	case 13:
		return "DITHER-DITHER_MOUT";
	case 12:
		return "OD-DITHER";
	case 11:
		return "GAMMA-OD";
	case 10:
		return "AAL-GAMMA";
	case 9:
		return "CCORR-AAL";
	case 8:
		return "COLOR-CCORR";
	case 7:
		return "COLOR_SEL-COLOR";
	case 6:
		return "OVL0_MOUT-WDMA0_SEL";
	case 5:
		return "OVL0_MOUT-COLOR_SEL";
	case 4:
		return "OVL0_SEL-OVL0_MOUT";
	case 3:
		return "OVL0_SOUT-OVL1_2L";
	case 2:
		return "OVL0_SOUT-OVL0_SEL";
	case 1:
		return "OVL0_2L-OVL0_SOUT";
	case 0:
		return "OVL0-OVL0_2L";
	default:
		return NULL;
	}
}

static char *ddp_signal_1(int bit)
{
	switch (bit) {
	case 0:
		return "UFOE_MOUT-DSI1_SEL";
	case 1:
		return "SPLIT-DSI0_SEL";
	case 2:
		return "SPLIT-DSI1_SEL";
	case 3:
		return "WDMA0_SEL-WDMA0";
	case 4:
		return "OVL1_2L-OVL1_SOUT";
	case 5:
		return "OVL1_SOUT-OVL1";
	case 6:
		return "OVL1_SOUT-OVL0_SEL";
	case 7:
		return "OVL1-OVL1_MOUT";
	case 8:
		return "OVL1_MOUT-RDMA1";
	case 9:
		return "OVL1_MOUT-WMDA1_SEL";
	case 10:
		return "OVL1_SOUT_ECO-OVL0_SEL";
	case 11:
		return "RDMA1-RDMA1_SOUT";
	case 12:
		return "RDMA1_SOUT-UFOE_SEL";
	case 13:
		return "RDMA1_SOUT-DSC_SEL";
	case 14:
		return "DSC_SEL-DSC";
	case 15:
		return "DSC-DSC_MOUT";
	case 16:
		return "DSC_MOUT-DSI0_SEL";
	case 17:
		return "DSC_MOUT-DSI1_SEL";
	case 18:
		return "DSC_MOUT-DPI0_SEL";
	case 19:
		return "DSC_MOUT-WDMA1_SEL";
	case 20:
		return "WDMA1_SEL-WDMA1";
	case 21:
		return "DSI0_SEL-DSI0";
	case 22:
		return "DSI1_SEL-DSI1";
	case 23:
		return "DPI0_SEL-DPI0";
	default:
		return NULL;
	}
}

static char *ddp_greq_name(int bit)
{
	switch (bit) {
	case 0:
		return "OVL0";
	case 1:
		return "OVL0_2L";
	case 2:
		return "RDMA0";
	case 3:
		return "WDMA0";
	case 16:
		return "OVL1";
	case 17:
		return "OVL1_2L";
	case 18:
		return "RDMA1";
	case 19:
		return "WDMA1";
	case 20:
		return "OD_R";
	case 21:
		return "OD_W";
	case 22:
		return "OVL0_2L_LARB5";
	case 25:
		return "WMDA0_LARB5";
	default:
		return NULL;
	}
}

static char *ddp_get_mutex_module_name(unsigned int bit)
{
	switch (bit) {
	case 10:
		return "ovl0";
	case 11:
		return "ovl1";
	case 12:
		return "ovl0_2l";
	case 13:
		return "rdma0";
	case 14:
		return "rdma1";
	case 15:
		return "ovl1_2l";
	case 16:
		return "wdma0";
	case 17:
		return "wdma1";
	case 18:
		return "color0";
	case 19:
		return "ccorr";
	case 20:
		return "aal";
	case 21:
		return "gamma";
	case 22:
		return "od";
	case 23:
		return "dither";
	case 24:
		return "ufoe";
	case 25:
		return "dsc";
	case 26:
		return "pwm0";
	default:
		return "mutex-unknown";
	}
}

char *ddp_get_fmt_name(DISP_MODULE_ENUM module, unsigned int fmt)
{
	if (module == DISP_MODULE_WDMA0) {
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
			DDPDUMP("ddp_get_fmt_name, unknown fmt=%d, module=%d\n", fmt, module);
			return "unknown";
		}
	} else if (module == DISP_MODULE_OVL0) {
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
			DDPDUMP("ddp_get_fmt_name, unknown fmt=%d, module=%d\n", fmt, module);
			return "unknown";
		}
	} else if (module == DISP_MODULE_RDMA0 || module == DISP_MODULE_RDMA1 || module == DISP_MODULE_RDMA2) {
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
			DDPDUMP("ddp_get_fmt_name, unknown fmt=%d, module=%d\n", fmt, module);
			return "unknown";
		}
	} else {
		DDPDUMP("ddp_get_fmt_name, unknown module=%d\n", module);
	}

	return "unknown";
}

static char *ddp_clock_0(int bit)
{
	switch (bit) {
	case 0:
		return "smi_common(cg), ";
	case 1:
		return "smi_larb0(cg), ";
	case 2:
		return "smi_larb5(cg), ";
	case 15:
		return "ovl0, ";
	case 16:
		return "ovl1, ";
	case 17:
		return "ovl0_2L, ";
	case 18:
		return "ovl1_2L, ";
	case 19:
		return "rdma0, ";
	case 20:
		return "rdma1, ";
	case 21:
		return "wdma0, ";
	case 22:
		return "wdma1, ";
	case 23:
		return "color, ";
	case 24:
		return "ccorr, ";
	case 25:
		return "aal, ";
	case 26:
		return "gamma, ";
	case 27:
		return "od, ";
	case 28:
		return "dither, ";
	case 29:
		return "ufoe_mout, ";
	case 30:
		return "dsc, ";
	case 31:
		return "split, ";
	default:
		return NULL;
	}
}

static char *ddp_clock_1(int bit)
{
	switch (bit) {
	case 0:
		return "dsi0_mm(cg), ";
	case 1:
		return "dsi0_interface(cg), ";
	case 2:
		return "dsi1_mm(cg), ";
	case 3:
		return "dsi1_interface(cg), ";
	case 4:
		return "dpi_mm(cg), ";
	case 5:
		return "dpi_interface, ";
	case 8:
		return "ovl0_mout, ";
	default:
		return NULL;
	}
}

static void mutex_dump_reg(void)
{
	DDPDUMP("== DISP MUTEX REGS ==\n");
	DDPDUMP("(0x000)M_INTEN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX_INTEN));
	DDPDUMP("(0x004)M_INTSTA=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX_INTSTA));
	DDPDUMP("(0x020)M0_EN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX0_EN));
	DDPDUMP("(0x028)M0_RST=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX0_RST));
	DDPDUMP("(0x02c)M0_MOD=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX0_MOD));
	DDPDUMP("(0x030)M0_SOF=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX0_SOF));
	DDPDUMP("(0x040)M1_EN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX1_EN));
	DDPDUMP("(0x048)M1_RST=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX1_RST));
	DDPDUMP("(0x04c)M1_MOD=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX1_MOD));
	DDPDUMP("(0x050)M1_SOF=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX1_SOF));
	DDPDUMP("(0x060)M2_EN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX2_EN));
	DDPDUMP("(0x068)M2_RST=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX2_RST));
	DDPDUMP("(0x06c)M2_MOD=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX2_MOD));
	DDPDUMP("(0x070)M2_SOF=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX2_SOF));
	DDPDUMP("(0x080)M3_EN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX3_EN));
	DDPDUMP("(0x088)M3_RST=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX3_RST));
	DDPDUMP("(0x08c)M3_MOD=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX3_MOD));
	DDPDUMP("(0x090)M3_SOF=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX3_SOF));
	DDPDUMP("(0x0a0)M4_EN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX4_EN));
	DDPDUMP("(0x0a8)M4_RST=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX4_RST));
	DDPDUMP("(0x0ac)M4_MOD=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX4_MOD));
	DDPDUMP("(0x0b0)M4_SOF=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX4_SOF));
	DDPDUMP("(0x0c0)M5_EN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX5_EN));
	DDPDUMP("(0x0c8)M5_RST=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX5_RST));
	DDPDUMP("(0x0cc)M5_MOD=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX5_MOD));
	DDPDUMP("(0x0d0)M5_SOF=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX5_SOF));
	DDPDUMP("(0x200)DEBUG_OUT_SEL=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DEBUG_OUT_SEL));
	return;
}


static void mutex_dump_analysis(void)
{
	int i = 0;
	int j = 0;
	char mutex_module[512] = { '\0' };
	char *p = NULL;
	int len = 0;
	unsigned int val;

	DDPDUMP("== DISP Mutex Analysis ==\n");
	for (i = 0; i < 5; i++) {
		p = mutex_module;
		len = 0;
		if (DISP_REG_GET(DISP_REG_CONFIG_MUTEX_MOD(i)) == 0)
			continue;

		val = DISP_REG_GET(DISP_REG_CONFIG_MUTEX_SOF(i));
		len = sprintf(p, "MUTEX%d :SOF=%s,EOF=%s,WAIT=%d,module=(",
			      i,
			      ddp_get_mutex_sof_name(REG_FLD_VAL_GET(SOF_FLD_MUTEX0_SOF, val)),
			      ddp_get_mutex_sof_name(REG_FLD_VAL_GET(SOF_FLD_MUTEX0_EOF, val)),
				REG_FLD_VAL_GET(SOF_FLD_MUTEX0_SOF_WAIT, val));

		p += len;
		for (j = 0; j < 32; j++) {
			unsigned int regval = DISP_REG_GET(DISP_REG_CONFIG_MUTEX_MOD(i));

			if ((regval & (1 << j))) {
				len = sprintf(p, "%s,", ddp_get_mutex_module_name(j));
				p += len;
			}
		}
		DDPDUMP("%s)\n", mutex_module);
	}
}

static void mmsys_config_dump_reg(void)
{
	DDPDUMP("== DISP MMSYS_Config REGS ==\n");
	DDPDUMP("MMSYS_INTEN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_INTEN));
	DDPDUMP("MMSYS_INTSTA=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_INTSTA));
	DDPDUMP("OVL0_MOUT_EN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_OVL0_MOUT_EN));
	DDPDUMP("OVL1_MOUT_EN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_OVL1_MOUT_EN));
	DDPDUMP("DITHER_MOUT_EN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_DITHER_MOUT_EN));
	DDPDUMP("UFOE_MOUT_EN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_UFOE_MOUT_EN));
	DDPDUMP("MMSYS_MOUT_RST=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_MOUT_RST));
	DDPDUMP("COLOR0_SIN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_COLOR0_SEL_IN));
	DDPDUMP("WDMA0_SIN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_WDMA0_SEL_IN));
	DDPDUMP("UFOE_SIN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_UFOE_SEL_IN));
	DDPDUMP("DSC_SIN=0x%x\n",  DISP_REG_GET(DISP_REG_CONFIG_DISP_DSC_SEL_IN));
	DDPDUMP("DSC_MOUT_EN=0x%x\n",  DISP_REG_GET(DISP_REG_CONFIG_DISP_DSC_MOUT_EN));
	DDPDUMP("DSI0_SIN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DSI0_SEL_IN));
	DDPDUMP("DSI1_SIN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DSI1_SEL_IN));
	DDPDUMP("DPI0_SIN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DPI0_SEL_IN));
	DDPDUMP("OVL0_SIN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_OVL0_SEL_IN));
	DDPDUMP("DISP_PATH_SOUT_SIN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_PATH0_SOUT_SEL_IN));
	DDPDUMP("RDMA0_SOUT_SIN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_RDMA0_SOUT_SEL_IN));
	DDPDUMP("RDMA1_SOUT_SIN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_RDMA1_SOUT_SEL_IN));
	DDPDUMP("OVL0_SOUT_SIN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_OVL0_SOUT_SEL_IN));
	DDPDUMP("OVL1_SOUT_SIN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_OVL1_SOUT_SEL_IN));

	DDPDUMP("(0x0F0)MM_MISC=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_MISC));
	DDPDUMP("(0x100)MM_CG_CON0=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON0));
	DDPDUMP("(0x110)MM_CG_CON1=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON1));
	DDPDUMP("(0x120)MM_HW_DCM_DIS0=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_HW_DCM_DIS0));
	DDPDUMP("(0x130)MM_HW_DCM_DIS1=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_HW_DCM_DIS1));
	DDPDUMP("(0x140)MM_SW0_RST_B=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_SW0_RST_B));
	DDPDUMP("(0x144)MM_SW1_RST_B=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_SW1_RST_B));
	DDPDUMP("(0x150)MM_LCM_RST_B=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_LCM_RST_B));
	DDPDUMP("(0x880)MM_DBG_OUT_SEL=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_DEBUG_OUT_SEL));
	DDPDUMP("(0x890)MM_DUMMY0=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_DUMMY0));
	DDPDUMP("(0x894)MM_DUMMY1=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_DUMMY1));
	DDPDUMP("(0x898)MM_DUMMY2=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_DUMMY2));
	DDPDUMP("(0x89C)MM_DUMMY3=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_DUMMY3));
	DDPDUMP("(0x8a0)DISP_VALID_0=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_DL_VALID_0));
	DDPDUMP("(0x8a4)DISP_VALID_1=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_DL_VALID_1));
	DDPDUMP("(0x8a8)DISP_READY_0=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_DL_READY_0));
	DDPDUMP("(0x8aC)DISP_READY_1=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_DL_READY_1));
	return;
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
	char *name;
	/* int len = 0; */

	unsigned int valid0 = DISP_REG_GET(DISP_REG_CONFIG_DISP_DL_VALID_0);
	unsigned int valid1 = DISP_REG_GET(DISP_REG_CONFIG_DISP_DL_VALID_1);
	unsigned int ready0 = DISP_REG_GET(DISP_REG_CONFIG_DISP_DL_READY_0);
	unsigned int ready1 = DISP_REG_GET(DISP_REG_CONFIG_DISP_DL_READY_1);
	unsigned int greq = DISP_REG_GET(DISP_REG_CONFIG_SMI_LARB0_GREQ);

	DDPDUMP("== DISP MMSYS_CONFIG ANALYSIS ==\n");
#if 0 /* TODO: mmsys clk?? */
	DDPDUMP("mmsys clock=0x%x, CG_CON0=0x%x, CG_CON1=0x%x\n",
		DISP_REG_GET(DISP_REG_CLK_CFG_0_MM_CLK),
		DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON0),
		DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON1));
	if ((DISP_REG_GET(DISP_REG_CLK_CFG_0_MM_CLK) >> 31) & 0x1)
		DDPERR("mmsys clock abnormal!!\n");
#endif

	reg = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON0);
	for (i = 0; i < 32; i++) {
		if ((reg & (1 << i)) == 0) {
			name = ddp_clock_0(i);
			if (name)
				strncat(clock_on, name, sizeof(clock_on));
		}
	}

	reg = DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON1);
	for (i = 0; i < 32; i++) {
		if ((reg & (1 << i)) == 0) {
			name = ddp_clock_1(i);
			if (name)
				strncat(clock_on, name, sizeof(clock_on));
		}
	}
	DDPDUMP("clock on modules:%s\n", clock_on);

	DDPDUMP("valid0=0x%x, valid1=0x%x, ready0=0x%x, ready1=0x%x, greq=0%x\n",
		valid0, valid1, ready0, ready1, greq);
	for (i = 0; i < 32; i++) {
		name = ddp_signal_0(i);
		if (!name)
			continue;

		pos = clock_on;

		if ((valid0 & (1 << i)))
			pos += sprintf(pos, "%s,", "v");
		else
			pos += sprintf(pos, "%s,", "n");

		if ((ready0 & (1 << i)))
			pos += sprintf(pos, "%s", "r");
		else
			pos += sprintf(pos, "%s", "n");

		pos += sprintf(pos, ": %s", name);

		DDPDUMP("%s\n", clock_on);
	}

	for (i = 0; i < 32; i++) {
		name = ddp_signal_1(i);
		if (!name)
			continue;

		pos = clock_on;

		if ((valid1 & (1 << i)))
			pos += sprintf(pos, "%s,", "v");
		else
			pos += sprintf(pos, "%s,", "n");

		if ((ready1 & (1 << i)))
			pos += sprintf(pos, "%s", "r");
		else
			pos += sprintf(pos, "%s", "n");

		pos += sprintf(pos, ": %s", name);

		DDPDUMP("%s\n", clock_on);
	}

	/* greq: 1 means SMI dose not grant, maybe SMI hang */
	if (greq)
		DDPDUMP("smi greq not grant module: (greq: 1 means SMI dose not grant, maybe SMI hang)");

	clock_on[0] = '\0';
	for (i = 0; i < 32; i++) {
		if (greq & (1 << i)) {
			name = ddp_greq_name(i);
			if (!name)
				continue;
			strncat(clock_on, name, sizeof(clock_on));
		}
	}
	DDPDUMP("%s\n", clock_on);
}

static void gamma_dump_reg(void)
{
	DDPDUMP("== DISP GAMMA REGS ==\n");
	DDPDUMP("(0x000)GA_EN=0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_EN));
	DDPDUMP("(0x004)GA_RESET=0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_RESET));
	DDPDUMP("(0x008)GA_INTEN=0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_INTEN));
	DDPDUMP("(0x00c)GA_INTSTA=0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_INTSTA));
	DDPDUMP("(0x010)GA_STATUS=0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_STATUS));
	DDPDUMP("(0x020)GA_CFG=0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_CFG));
	DDPDUMP("(0x024)GA_IN_COUNT=0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_INPUT_COUNT));
	DDPDUMP("(0x028)GA_OUT_COUNT=0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_OUTPUT_COUNT));
	DDPDUMP("(0x02c)GA_CHKSUM=0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_CHKSUM));
	DDPDUMP("(0x030)GA_SIZE=0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_SIZE));
	DDPDUMP("(0x0c0)GA_DUMMY_REG=0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_DUMMY_REG));
	DDPDUMP("(0x800)GA_LUT=0x%x\n", DISP_REG_GET(DISP_REG_GAMMA_LUT));
}

static void gamma_dump_analysis(void)
{
	DDPDUMP("== DISP GAMMA ANALYSIS ==\n");
	DDPDUMP("gamma: en=%d, w=%d, h=%d, in_p_cnt=%d, in_l_cnt=%d, out_p_cnt=%d, out_l_cnt=%d\n",
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
	DDPDUMP("== DISP MERGE REGS ==\n");
	DDPDUMP("(0x000)MERGE_EN=0x%x\n", DISP_REG_GET(DISP_REG_MERGE_ENABLE));
	DDPDUMP("(0x004)MERGE_SW_RESET=0x%x\n", DISP_REG_GET(DISP_REG_MERGE_SW_RESET));
	DDPDUMP("(0x008)MERGE_DEBUG=0x%x\n", DISP_REG_GET(DISP_REG_MERGE_DEBUG));
}

static void merge_dump_analysis(void)
{
	DDPDUMP("== DISP MERGE ANALYSIS ==\n");
	DDPDUMP("merge: en=%d, debug=0x%x\n", DISP_REG_GET(DISP_REG_MERGE_ENABLE),
		DISP_REG_GET(DISP_REG_MERGE_DEBUG));
}


static void color_dump_reg(DISP_MODULE_ENUM module)
{
	int index = 0;

	if (DISP_MODULE_COLOR0 == module) {
		index = 0;
	} else if (DISP_MODULE_COLOR1 == module) {
		DDPDUMP("error: DISP COLOR%d dose not exist!\n", index);
		return;
	}
	DDPDUMP("== DISP COLOR%d REGS ==\n", index);
	DDPDUMP("(0x400)COLOR_CFG_MAIN=0x%x\n", DISP_REG_GET(DISP_COLOR_CFG_MAIN));
	DDPDUMP("(0x404)COLOR_PXL_CNT_MAIN=0x%x\n", DISP_REG_GET(DISP_COLOR_PXL_CNT_MAIN));
	DDPDUMP("(0x408)COLOR_LINE_CNT_MAIN=0x%x\n", DISP_REG_GET(DISP_COLOR_LINE_CNT_MAIN));
	DDPDUMP("(0xc00)COLOR_START=0x%x\n", DISP_REG_GET(DISP_COLOR_START));
	DDPDUMP("(0xc28)DISP_COLOR_CK_ON=0x%x\n", DISP_REG_GET(DISP_COLOR_CK_ON));
	DDPDUMP("(0xc50)COLOR_INTER_IP_W=0x%x\n", DISP_REG_GET(DISP_COLOR_INTERNAL_IP_WIDTH));
	DDPDUMP("(0xc54)COLOR_INTER_IP_H=0x%x\n", DISP_REG_GET(DISP_COLOR_INTERNAL_IP_HEIGHT));
	return;
}

static void color_dump_analysis(DISP_MODULE_ENUM module)
{
	int index = 0;

	if (DISP_MODULE_COLOR0 == module) {
		index = 0;
	} else if (DISP_MODULE_COLOR1 == module) {
		DDPDUMP("error: DISP COLOR%d dose not exist!\n", index);
		return;
	}
	DDPDUMP("== DISP COLOR%d ANALYSIS ==\n", index);
	DDPDUMP("color%d: bypass=%d, w=%d, h=%d, pixel_cnt=%d, line_cnt=%d,\n",
		index,
		(DISP_REG_GET(DISP_COLOR_CFG_MAIN) >> 7) & 0x1,
		DISP_REG_GET(DISP_COLOR_INTERNAL_IP_WIDTH),
		DISP_REG_GET(DISP_COLOR_INTERNAL_IP_HEIGHT),
		DISP_REG_GET(DISP_COLOR_PXL_CNT_MAIN) & 0xffff,
		(DISP_REG_GET(DISP_COLOR_LINE_CNT_MAIN) >> 16) & 0x1fff);

	return;
}

static void aal_dump_reg(void)
{
	DDPDUMP("== DISP AAL REGS ==\n");
	DDPDUMP("(0x000)AAL_EN=0x%x\n", DISP_REG_GET(DISP_AAL_EN));
	DDPDUMP("(0x008)AAL_INTEN=0x%x\n", DISP_REG_GET(DISP_AAL_INTEN));
	DDPDUMP("(0x00c)AAL_INTSTA=0x%x\n", DISP_REG_GET(DISP_AAL_INTSTA));
	DDPDUMP("(0x020)AAL_CFG=0x%x\n", DISP_REG_GET(DISP_AAL_CFG));
	DDPDUMP("(0x024)AAL_IN_CNT=0x%x\n", DISP_REG_GET(DISP_AAL_IN_CNT));
	DDPDUMP("(0x028)AAL_OUT_CNT=0x%x\n", DISP_REG_GET(DISP_AAL_OUT_CNT));
	DDPDUMP("(0x030)AAL_SIZE=0x%x\n", DISP_REG_GET(DISP_AAL_SIZE));
	DDPDUMP("(0x20c)AAL_CABC_00=0x%x\n", DISP_REG_GET(DISP_AAL_CABC_00));
	DDPDUMP("(0x214)AAL_CABC_02=0x%x\n", DISP_REG_GET(DISP_AAL_CABC_02));
	DDPDUMP("(0x20c)AAL_STATUS_00=0x%x\n", DISP_REG_GET(DISP_AAL_STATUS_00));
	DDPDUMP("(0x210)AAL_STATUS_01=0x%x\n", DISP_REG_GET(DISP_AAL_STATUS_00 + 0x4));
	DDPDUMP("(0x2a0)AAL_STATUS_31=0x%x\n", DISP_REG_GET(DISP_AAL_STATUS_32 - 0x4));
	DDPDUMP("(0x2a4)AAL_STATUS_32=0x%x\n", DISP_REG_GET(DISP_AAL_STATUS_32));
	DDPDUMP("(0x3b0)AAL_DRE_MAPPING_00=0x%x\n", DISP_REG_GET(DISP_AAL_DRE_MAPPING_00));
	return;
}

static void aal_dump_analysis(void)
{
	DDPDUMP("== DISP AAL ANALYSIS ==\n");
	DDPDUMP("aal: bypass=%d, relay=%d, en=%d, w=%d, h=%d, in(%d,%d),out(%d,%d)\n",
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
	DDPDUMP("== DISP PWM%d REGS ==\n", index);
	DDPDUMP("(0x000)PWM_EN=0x%x\n", DISP_REG_GET(reg_base + DISP_PWM_EN_OFF));
	DDPDUMP("(0x008)PWM_CON_0=0x%x\n", DISP_REG_GET(reg_base + DISP_PWM_CON_0_OFF));
	DDPDUMP("(0x010)PWM_CON_1=0x%x\n", DISP_REG_GET(reg_base + DISP_PWM_CON_1_OFF));
	DDPDUMP("(0x028)PWM_DEBUG=0x%x\n", DISP_REG_GET(reg_base + 0x28));
	return;
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
	DDPDUMP("== DISP PWM%d ANALYSIS ==\n", index);
#if 0 /* TODO: clk reg?? */
	DDPDUMP("pwm clock=%d\n", (DISP_REG_GET(DISP_REG_CLK_CFG_1_CLR) >> 7) & 0x1);
#endif

	return;
}

static void od_dump_reg(void)
{
	DDPDUMP("== DISP OD REGS ==\n");
	DDPDUMP("(00)EN=0x%x\n", DISP_REG_GET(DISP_REG_OD_EN));
	DDPDUMP("(04)RESET=0x%x\n", DISP_REG_GET(DISP_REG_OD_RESET));
	DDPDUMP("(08)INTEN=0x%x\n", DISP_REG_GET(DISP_REG_OD_INTEN));
	DDPDUMP("(0C)INTSTA=0x%x\n", DISP_REG_GET(DISP_REG_OD_INTSTA));
	DDPDUMP("(10)STATUS=0x%x\n", DISP_REG_GET(DISP_REG_OD_STATUS));
	DDPDUMP("(20)CFG=0x%x\n", DISP_REG_GET(DISP_REG_OD_CFG));
	DDPDUMP("(24)INPUT_COUNT=0x%x\n", DISP_REG_GET(DISP_REG_OD_INPUT_COUNT));
	DDPDUMP("(28)OUTPUT_COUNT=0x%x\n", DISP_REG_GET(DISP_REG_OD_OUTPUT_COUNT));
	DDPDUMP("(2C)CHKSUM=0x%x\n", DISP_REG_GET(DISP_REG_OD_CHKSUM));
	DDPDUMP("(30)SIZE=0x%x\n", DISP_REG_GET(DISP_REG_OD_SIZE));
	DDPDUMP("(40)HSYNC_WIDTH=0x%x\n", DISP_REG_GET(DISP_REG_OD_HSYNC_WIDTH));
	DDPDUMP("(44)VSYNC_WIDTH=0x%x\n", DISP_REG_GET(DISP_REG_OD_VSYNC_WIDTH));
	DDPDUMP("(48)MISC=0x%x\n", DISP_REG_GET(DISP_REG_OD_MISC));
	DDPDUMP("(C0)DUMMY_REG=0x%x\n", DISP_REG_GET(DISP_REG_OD_DUMMY_REG));
	return;
}

static void od_dump_analysis(void)
{
	DDPDUMP("== DISP OD ANALYSIS ==\n");
	DDPDUMP("od: w=%d, h=%d, bypass=%d\n",
		(DISP_REG_GET(DISP_REG_OD_SIZE) >> 16) & 0xffff,
		DISP_REG_GET(DISP_REG_OD_SIZE) & 0xffff, DISP_REG_GET(DISP_REG_OD_CFG) & 0x1);

	return;
}

static void ccorr_dump_reg(void)
{
	DDPDUMP("== DISP CCORR REGS ==\n");
	DDPDUMP("(00)EN=0x%x\n", DISP_REG_GET(DISP_REG_CCORR_EN));
	DDPDUMP("(20)CFG=0x%x\n", DISP_REG_GET(DISP_REG_CCORR_CFG));
	DDPDUMP("(24)IN_CNT=0x%x\n", DISP_REG_GET(DISP_REG_CCORR_IN_CNT));
	DDPDUMP("(28)OUT_CNT=0x%x\n", DISP_REG_GET(DISP_REG_CCORR_OUT_CNT));
	DDPDUMP("(30)SIZE=0x%x\n", DISP_REG_GET(DISP_REG_CCORR_SIZE));

}

static void ccorr_dump_analyze(void)
{
	DDPDUMP("ccorr: en=%d, config=%d, w=%d, h=%d, in_p_cnt=%d, in_l_cnt=%d, out_p_cnt=%d, out_l_cnt=%d\n",
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
	DDPDUMP("== DISP DITHER REGS ==\n");
	DDPDUMP("(00)EN=0x%x\n", DISP_REG_GET(DISP_REG_DITHER_EN));
	DDPDUMP("(20)CFG=0x%x\n", DISP_REG_GET(DISP_REG_DITHER_CFG));
	DDPDUMP("(24)IN_CNT=0x%x\n", DISP_REG_GET(DISP_REG_DITHER_IN_CNT));
	DDPDUMP("(28)OUT_CNT=0x%x\n", DISP_REG_GET(DISP_REG_DITHER_OUT_CNT));
	DDPDUMP("(30)SIZE=0x%x\n", DISP_REG_GET(DISP_REG_DITHER_SIZE));
}

static void dither_dump_analyze(void)
{
	DDPDUMP
	    ("dither: en=%d, config=%d, w=%d, h=%d, in_p_cnt=%d, in_l_cnt=%d, out_p_cnt=%d, out_l_cnt=%d\n",
	     DISP_REG_GET(DISPSYS_DITHER_BASE + 0x000), DISP_REG_GET(DISPSYS_DITHER_BASE + 0x020),
	     (DISP_REG_GET(DISP_REG_DITHER_SIZE) >> 16) & 0x1fff,
	     DISP_REG_GET(DISP_REG_DITHER_SIZE) & 0x1fff,
	     DISP_REG_GET(DISP_REG_DITHER_IN_CNT) & 0x1fff,
	     (DISP_REG_GET(DISP_REG_DITHER_IN_CNT) >> 16) & 0x1fff,
	     DISP_REG_GET(DISP_REG_DITHER_OUT_CNT) & 0x1fff,
	     (DISP_REG_GET(DISP_REG_DITHER_OUT_CNT) >> 16) & 0x1fff);
}

static void ufoe_dump(void)
{
	DDPDUMP("== DISP UFOE REGS ==\n");
	DDPDUMP("(0x000)UFOE_START=0x%x\n", DISP_REG_GET(DISP_REG_UFO_START));
	DDPDUMP("(0x020)UFOE_PAD=0x%x\n", DISP_REG_GET(DISP_REG_UFO_CR0P6_PAD));
	DDPDUMP("(0x050)UFOE_WIDTH=0x%x\n", DISP_REG_GET(DISP_REG_UFO_FRAME_WIDTH));
	DDPDUMP("(0x054)UFOE_HEIGHT=0x%x\n", DISP_REG_GET(DISP_REG_UFO_FRAME_HEIGHT));
	DDPDUMP("(0x100)UFOE_CFG0=0x%x\n", DISP_REG_GET(DISP_REG_UFO_CFG_0B));
	DDPDUMP("(0x104)UFOE_CFG1=0x%x\n", DISP_REG_GET(DISP_REG_UFO_CFG_1B));
}

static void dsc_dump(void)
{
	u32 i = 0;

	DDPDUMP("== DISP DSC REGS ==\n");
	DDPDUMP("(0x000)DSC_START=0x%x\n", DISP_REG_GET(DISP_REG_DSC_CON));
	DDPDUMP("(0x000)DSC_SLICE_WIDTH=0x%x\n", DISP_REG_GET(DISP_REG_DSC_SLICE_W));
	DDPDUMP("(0x000)DSC_SLICE_HIGHT=0x%x\n", DISP_REG_GET(DISP_REG_DSC_SLICE_H));
	DDPDUMP("(0x000)DSC_WIDTH=0x%x\n", DISP_REG_GET(DISP_REG_DSC_PIC_W));
	DDPDUMP("(0x000)DSC_HEIGHT=0x%x\n", DISP_REG_GET(DISP_REG_DSC_PIC_H));
	DDPDUMP("-- Start dump dsc registers --\n");
	for (i = 0; i < 204; i += 16) {
		DDPDUMP("DSC+%x: 0x%x 0x%x 0x%x 0x%x\n", i, DISP_REG_GET(DISPSYS_DSC_BASE + i),
				DISP_REG_GET(DISPSYS_DSC_BASE + i + 0x4), DISP_REG_GET(DISPSYS_DSC_BASE + i + 0x8),
				DISP_REG_GET(DISPSYS_DSC_BASE + i + 0xc));
	}
}

static void dsi_dump_reg(DISP_MODULE_ENUM module)
{
	DSI_DumpRegisters(module, 1);
#if 0
	if (DISP_MODULE_DSI0) {
		int i = 0;

		DDPDUMP("== DISP DSI0 REGS ==\n");
		for (i = 0; i < 25 * 16; i += 16) {
			DDPDUMP("DSI0+%04x: 0x%08x 0x%08x 0x%08x 0x%08x\n", i,
			       INREG32(DISPSYS_DSI0_BASE + i), INREG32(DISPSYS_DSI0_BASE + i + 0x4),
			       INREG32(DISPSYS_DSI0_BASE + i + 0x8),
			       INREG32(DISPSYS_DSI0_BASE + i + 0xc));
		}
		DDPDUMP("DSI0 CMDQ+0x200: 0x%08x 0x%08x 0x%08x 0x%08x\n",
		       INREG32(DISPSYS_DSI0_BASE + 0x200), INREG32(DISPSYS_DSI0_BASE + 0x200 + 0x4),
		       INREG32(DISPSYS_DSI0_BASE + 0x200 + 0x8),
		       INREG32(DISPSYS_DSI0_BASE + 0x200 + 0xc));
	}
#endif
}

static void dpi_dump_reg(void)
{
	int i;

	DDPDUMP("-- Start dump DPI registers --\n");

	for (i = 0; i <= 0x40; i += 4)
		DDPDUMP("DPI+%04x: 0x%08x\n", i, INREG32(DISPSYS_DPI_BASE + i));

	for (i = 0x68; i <= 0x7C; i += 4)
		DDPDUMP("DPI+%04x: 0x%08x\n", i, INREG32(DISPSYS_DPI_BASE + i));

	DDPDUMP("DPI+Color Bar    : 0x%04x : 0x%08x\n", 0xF00, INREG32(DISPSYS_DPI_BASE + 0xF00));
	DDPDUMP("DPI MMSYS_CG_CON0: 0x%08x\n", INREG32(DISP_REG_CONFIG_MMSYS_CG_CON0));
	DDPDUMP("DPI MMSYS_CG_CON1: 0x%08x\n", INREG32(DISP_REG_CONFIG_MMSYS_CG_CON1));
}

static void dpi_dump_analysis(void)
{
#if 0
	DDPDUMP("== DISP DPI ANALYSIS ==\n");
	/* TODO: mmsys clk?? */
	DDPDUMP("DPI clock=0x%x\n", DISP_REG_GET(DISP_REG_CLK_CFG_6_DPI));
	DDPDUMP("DPI  clock_clear=%d\n", (DISP_REG_GET(DISP_REG_CLK_CFG_6_CLR) >> 7) & 0x1);
#endif
}

static int split_dump_regs(void)
{
	DDPDUMP("== DISP SPLIT0 REGS ==\n");
	DDPDUMP("(0x000)S_ENABLE=0x%x\n", DISP_REG_GET(DISP_REG_SPLIT_ENABLE));
	DDPDUMP("(0x004)S_SW_RST=0x%x\n", DISP_REG_GET(DISP_REG_SPLIT_SW_RESET));
	DDPDUMP("(0x008)S_DEBUG=0x%x\n", DISP_REG_GET(DISP_REG_SPLIT_DEBUG));
	return 0;
}

static char *split_state(unsigned int state)
{
	switch (state) {
	case 1:
		return "idle";
	case 2:
		return "wait";
	case 4:
		return "busy";
	default:
		return "unknown";
	}
	return "unknown";
}


static int split_dump_analysis(void)
{
	unsigned int pixel = DISP_REG_GET_FIELD(DEBUG_FLD_IN_PIXEL_CNT, DISP_REG_SPLIT_DEBUG);
	unsigned int state = DISP_REG_GET_FIELD(DEBUG_FLD_SPLIT_FSM, DISP_REG_SPLIT_DEBUG);

	DDPDUMP("== DISP SPLIT0 ANALYSIS ==\n");
	DDPDUMP("cur_pixel %u, state %s\n", pixel, split_state(state));
	return 0;
}

int ddp_dump_reg(DISP_MODULE_ENUM module)
{
	switch (module) {
	case DISP_MODULE_WDMA0:
		wdma_dump_reg(module);
		break;
	case DISP_MODULE_RDMA0:
	case DISP_MODULE_RDMA1:
	case DISP_MODULE_RDMA2:
		rdma_dump_reg(module);
		break;
	case DISP_MODULE_OVL0:
	case DISP_MODULE_OVL1:
	case DISP_MODULE_OVL0_2L:
	case DISP_MODULE_OVL1_2L:
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
		split_dump_regs();
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
	case DISP_MODULE_OD:
		od_dump_reg();
		break;
	case DISP_MODULE_DSI0:
	case DISP_MODULE_DSI1:
	case DISP_MODULE_DSIDUAL:
		dsi_dump_reg(module);
		break;
	case DISP_MODULE_DPI:
		dpi_dump_reg();
		break;
	case DISP_MODULE_CCORR:
		ccorr_dump_reg();
		break;
	case DISP_MODULE_DITHER:
		dither_dump_reg();
		break;
	case DISP_MODULE_UFOE:
		ufoe_dump();
		break;
	case DISP_MODULE_DSC:
		dsc_dump();
		break;
	default:
		DDPDUMP("no dump_reg for module %s(%d)\n", ddp_get_module_name(module), module);
	}
	return 0;
}

int ddp_dump_analysis(DISP_MODULE_ENUM module)
{
	switch (module) {
	case DISP_MODULE_WDMA0:
		wdma_dump_analysis(module);
		break;
	case DISP_MODULE_RDMA0:
	case DISP_MODULE_RDMA1:
	case DISP_MODULE_RDMA2:
		rdma_dump_analysis(module);
		break;
	case DISP_MODULE_OVL0:
	case DISP_MODULE_OVL1:
	case DISP_MODULE_OVL0_2L:
	case DISP_MODULE_OVL1_2L:
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
		split_dump_analysis();
		break;
	case DISP_MODULE_COLOR0:
	case DISP_MODULE_COLOR1:
		color_dump_analysis(module);
		break;
	case DISP_MODULE_AAL:
		aal_dump_analysis();
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
		dsi_analysis(module);
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
		DDPDUMP("no dump_analysis for module %s(%d)\n", ddp_get_module_name(module),
			module);
	}
	return 0;
}

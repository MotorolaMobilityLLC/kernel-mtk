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
#include "disp_helper.h"

static const char *ddp_signal[3][32] = {
{
		"aal0_to_gamma0",
		"aal1_to_gamma1",
		"ccorr0_to_aal0",
		"ccorr1_to_aal1",
		"color0_to_ccorr0",
		"color0_sel_to_color0",
		"color1_to_ccorr1",
		"color1_sel_to_color1",
		"path0_sel_to_path0_sout",
		"path0_sout_out_0_to_ufoe_sel_in_0",
		"path0_sout_out_1_to_dsc_sel_in_0",
		"path1_sel_to_path1",
		"path1_sout_out_0_to_dsc_sel_in_1",
		"path1_sout_out_1_to_ufoe_sel_in_1",
		"rdma0_to_rdma0_sout",
		"rdma0_sout_out_0_to_path0_sel_in_0",
		"rdma0_sout_out_1_to_color0_sel_in_0",
		"rdma0_sout_out_2_to_dsi0_sel_in_2",
		"rdma0_sout_out_3_to_dsi1_sel_in_1",
		"rdma0_sout_out_4_to_dpi0_sel_in_1",
		"rdma1_to_rdma1_sout",
		"rdma1_sout_out_0_to_path1_sel_in_0",
		"rdma1_sout_out_1_to_color1_sel_in_0",
		"rdma1_sout_out_2_to_dsi0_sel_in_4",
		"rdma1_sout_out_3_to_dsi1_sel_in_4",
		"rdma1_sout_out_4_to_dpi0_sel_in_3",
		"rdma2_to_dpi0_sel_in_4",
		"split_out_0_to_dsi0_1",
		"split_1_to_dsi1_0",
		"wdma0_sel_to_wdma0",
		"wdma1_sel_to_wdma1",
		"dither0_to_dither0_mout",
	},
	{
		"dither0_mout_out_0_to_rdma0",
		"dither0_mout_out_1_to_path0_sel_in_1",
		"dither0_mout_out_2_to_wdma0_sel_in_1",
		"dither1_to_dither1_mout",
		"dither1_mout_out_0_to_rdma1",
		"dither1_mout_out_1_to_path1_sel_in_1",
		"dither1_mout_out_2_to_wdma1_sel_in_1",
		"dpi0_sel_to_dpi0",
		"dsc_mout_out_0_to_dsi0_sel_in_3",
		"dsc_mout_out_1_to_dsi1_sel_in_2",
		"dsc_mout_out_2_to_dpi0_sel_in_2",
		"dsc_mout_out_3_to_wdma1_sel_in_2",
		"dsc_sel_to_dsc_wrap",
		"dsi0_sel_to_dsi0",
		"dsi1_sel_to_dsi1",
		"gamma0_to_od",
		"gamma1_to_dither1",
		"od_to_dither0",
		"ovl0_2L_to_ovl0_sout",
		"ovl0_to_ovl0_mout",
		"ovl0_mout_out_0_to_ovl0_2L",
		"ovl0_mout_out_1_to_wdma0_sel_in_3",
		"ovl0_pq_mout_out_0_to_color0_sel_in_1",
		"ovl0_pq_mout_out_1_to_wdma0_sel_in_0",
		"ovl0_sel_to_ovl_pq_mout",
		"ovl0_sout_out_0_to_ovl0_sel_in_0",
		"ovl0_sout_out_1_to_ovl1",
		"ovl0_sout_out_2_to_ovl1_2L_sel_in_0",
		"ovl1_2L_to_ovl1_pq_mout",
		"ovl1_2L_sel_to_ovl1_2L",
		"ovl1_2L_to_ovl1_mout",
		"ovl1_2L_mout_out_0_to_ovl1_2L_sel_in_1",
	},
	{
		"ovl1_mout_out_1_to_ovl0_sel_in_2",
		"ovl1_mout_out_2_to_wdma1_sel_in_3",
		"ovl1_mout_out_3_to_color1_sel_in_2",
		"ovl1_pq_mout_out_0_to_color1_sel_in_1",
		"ovl1_pq_mout_out_1_to_wdma1_sel_in_0",
		"ovl1_pq_mout_out_2_to_ovl0_sel_in_1",
		"ufoe_to_ufoe_mout",
		"ufoe_mout_out_0_to_dsi0_sel_in_0",
		"ufoe_mout_out_1_to_split",
		"ufoe_mout_out_2_to_dpi0_sel_in_0",
		"ufoe_mout_out_3_to_wdma0_sel_in_2",
		"ufoe_mout_out_4_to_dsi1_sel_in_3",
		"ufoe_sel_to_ufoe",
		"dsc_wrap_to_dsc_mout",
	}

};

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
	case 4:
		return "MDP_RDMA0";
	case 5:
		return "MDP_WDMA";
	case 6:
		return "MDP_WROT0";
	case 16:
		return "OVL1";
	case 17:
		return "OVL1_2L";
	case 18:
		return "RDMA1";
	case 19:
		return "RDMA2";
	case 20:
		return "WDMA1";
	case 21:
		return "OD_R";
	case 22:
		return "OD_W";
	case 23:
		return "OVL0_2L_LARB4";
	case 26:
		return "WMDA0_LARB5";
	default:
		return NULL;
	}
}

static char *ddp_get_mutex_module0_name(unsigned int bit)
{
		switch (bit) {
		case 0:  return "rdma0";
		case 1:  return "rdma1";
		case 2:  return "mdp_rdma0";
		case 4:  return "mdp_rsz0";
		case 5:  return "mdp_rsz1";
		case 6:  return "mdp_tdshp";
		case 7: return "mdp_wrot0";
		case 8: return "mdp_wrot1";
		case 9: return "ovl0";
		case 10: return "ovl0_2L";
		case 11: return "ovl1_2L";
		case 12: return "wdma0";
		case 13: return "color0";
		case 14: return "ccorr0";
		case 15: return "aal0";
		case 16: return "gamma0";
		case 17: return "dither0";
		case 18: return "PWM0";
		case 19: return "DSI";
		case 20: return "DPI";
		default:
			return "mutex-unknown";
	}
}
/*
static char *ddp_get_mutex_module1_name(unsigned int bit)
{
	switch (bit) {
	case 0:  return "pwm";
	case 1:  return "dsi0";
	case 2:  return "dsi1";
	default:
			return "mutex-unknown";
	}
}
*/
char *ddp_get_fmt_name(enum DISP_MODULE_ENUM module, unsigned int fmt)
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
		return "smi_common(cg),";
	case 1:
		return "smi_larb0(cg),";
	case 2:
		return "smi_larb4(cg),";
	case 15:
		return "ovl0,";
	case 16:
		return "ovl1,";
	case 17:
		return "ovl0_2L,";
	case 18:
		return "ovl1_2L,";
	case 19:
		return "rdma0,";
	case 20:
		return "rdma1,";
	case 21:
		return "rdma2,";
	case 22:
		return "wdma0,";
	case 23:
		return "wdma1,";
	case 24:
		return "color,";
	case 25:
		return "color1,";
	case 26:
		return "ccorr,";
	case 27:
		return "ccorr1,";
	case 28:
		return "aal,";
	case 29:
		return "aal1,";
	case 30:
		return "gamma,";
	case 31:
		return "gamma1,";
	default:
		return NULL;
	}
}

static char *ddp_clock_1(int bit)
{
	switch (bit) {
	case 0:
		return "od,";
	case 1:
		return "dither,";
	case 2:
		return "dither1,";
	case 3:
		return "ufoe,";
	case 4:
		return "dsc,";
	case 5:
		return "split,";
	case 6:
		return "dsi0_mm(cg),";
	case 7:
		return "dsi0_interface(cg),";
	case 8:
		return "dsi1_mm(cg),";
	case 9:
		return "dsi1_interface(cg),";
	case 10:
		return "dpi_mm(cg),";
	case 11:
		return "dpi_interface(cg),";
	case 14:
		return "ovl0_mout,";
	default:
		return NULL;
	}
}

static void mutex_dump_reg(void)
{
	if (disp_helper_get_option(DISP_OPT_REG_PARSER_RAW_DUMP)) {
		DDPDUMP("== START: DISP MUTEX REGS ==\n");
		DDPDUMP("MUTEX: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x0, INREG32(DISPSYS_MUTEX_BASE + 0x0),
			0x4, INREG32(DISPSYS_MUTEX_BASE + 0x4),
			0x8, INREG32(DISPSYS_MUTEX_BASE + 0x8),
			0xC, INREG32(DISPSYS_MUTEX_BASE + 0xC));
		DDPDUMP("MUTEX: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x10, INREG32(DISPSYS_MUTEX_BASE + 0x10),
			0x14, INREG32(DISPSYS_MUTEX_BASE + 0x14),
			0x18, INREG32(DISPSYS_MUTEX_BASE + 0x18),
			0x1C, INREG32(DISPSYS_MUTEX_BASE + 0x1C));
		DDPDUMP("MUTEX: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x020, INREG32(DISPSYS_MUTEX_BASE + 0x020),
			0x024, INREG32(DISPSYS_MUTEX_BASE + 0x024),
			0x028, INREG32(DISPSYS_MUTEX_BASE + 0x028),
			0x02C, INREG32(DISPSYS_MUTEX_BASE + 0x02C));
		DDPDUMP("MUTEX: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x030, INREG32(DISPSYS_MUTEX_BASE + 0x030),
			0x034, INREG32(DISPSYS_MUTEX_BASE + 0x034),
			0x040, INREG32(DISPSYS_MUTEX_BASE + 0x040),
			0x044, INREG32(DISPSYS_MUTEX_BASE + 0x044));
		DDPDUMP("MUTEX: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x048, INREG32(DISPSYS_MUTEX_BASE + 0x048),
			0x04C, INREG32(DISPSYS_MUTEX_BASE + 0x04C),
			0x050, INREG32(DISPSYS_MUTEX_BASE + 0x050),
			0x054, INREG32(DISPSYS_MUTEX_BASE + 0x054));
		DDPDUMP("MUTEX: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x060, INREG32(DISPSYS_MUTEX_BASE + 0x060),
			0x064, INREG32(DISPSYS_MUTEX_BASE + 0x064),
			0x068, INREG32(DISPSYS_MUTEX_BASE + 0x068),
			0x06C, INREG32(DISPSYS_MUTEX_BASE + 0x06C));
		DDPDUMP("MUTEX: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x070, INREG32(DISPSYS_MUTEX_BASE + 0x070),
			0x074, INREG32(DISPSYS_MUTEX_BASE + 0x074),
			0x080, INREG32(DISPSYS_MUTEX_BASE + 0x080),
			0x084, INREG32(DISPSYS_MUTEX_BASE + 0x084));
		DDPDUMP("MUTEX: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x088, INREG32(DISPSYS_MUTEX_BASE + 0x088),
			0x08C, INREG32(DISPSYS_MUTEX_BASE + 0x08C),
			0x090, INREG32(DISPSYS_MUTEX_BASE + 0x090),
			0x094, INREG32(DISPSYS_MUTEX_BASE + 0x094));
		DDPDUMP("MUTEX: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x0A0, INREG32(DISPSYS_MUTEX_BASE + 0x0A0),
			0x0A4, INREG32(DISPSYS_MUTEX_BASE + 0x0A4),
			0x0A8, INREG32(DISPSYS_MUTEX_BASE + 0x0A8),
			0x0AC, INREG32(DISPSYS_MUTEX_BASE + 0x0AC));
		DDPDUMP("MUTEX: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x0B0, INREG32(DISPSYS_MUTEX_BASE + 0x0B0),
			0x0B4, INREG32(DISPSYS_MUTEX_BASE + 0x0B4),
			0x0C0, INREG32(DISPSYS_MUTEX_BASE + 0x0C0),
			0x0C4, INREG32(DISPSYS_MUTEX_BASE + 0x0C4));
		DDPDUMP("MUTEX: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x0C8, INREG32(DISPSYS_MUTEX_BASE + 0x0C8),
			0x0CC, INREG32(DISPSYS_MUTEX_BASE + 0x0CC),
			0x0D0, INREG32(DISPSYS_MUTEX_BASE + 0x0D0),
			0x0D4, INREG32(DISPSYS_MUTEX_BASE + 0x0D4));
		DDPDUMP("MUTEX: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x0E0, INREG32(DISPSYS_MUTEX_BASE + 0x0E0),
			0x0E4, INREG32(DISPSYS_MUTEX_BASE + 0x0E4),
			0x0E8, INREG32(DISPSYS_MUTEX_BASE + 0x0E8),
			0x0EC, INREG32(DISPSYS_MUTEX_BASE + 0x0EC));
		DDPDUMP("MUTEX: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x0F0, INREG32(DISPSYS_MUTEX_BASE + 0x0F0),
			0x0F4, INREG32(DISPSYS_MUTEX_BASE + 0x0F4),
			0x100, INREG32(DISPSYS_MUTEX_BASE + 0x100),
			0x104, INREG32(DISPSYS_MUTEX_BASE + 0x104));
		DDPDUMP("MUTEX: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x108, INREG32(DISPSYS_MUTEX_BASE + 0x108),
			0x10C, INREG32(DISPSYS_MUTEX_BASE + 0x10C),
			0x110, INREG32(DISPSYS_MUTEX_BASE + 0x110),
			0x114, INREG32(DISPSYS_MUTEX_BASE + 0x114));
		DDPDUMP("MUTEX: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x120, INREG32(DISPSYS_MUTEX_BASE + 0x120),
			0x124, INREG32(DISPSYS_MUTEX_BASE + 0x124),
			0x128, INREG32(DISPSYS_MUTEX_BASE + 0x128),
			0x12C, INREG32(DISPSYS_MUTEX_BASE + 0x12C));
		DDPDUMP("MUTEX: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x130, INREG32(DISPSYS_MUTEX_BASE + 0x130),
			0x134, INREG32(DISPSYS_MUTEX_BASE + 0x134),
			0x140, INREG32(DISPSYS_MUTEX_BASE + 0x140),
			0x144, INREG32(DISPSYS_MUTEX_BASE + 0x144));
		DDPDUMP("MUTEX: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x148, INREG32(DISPSYS_MUTEX_BASE + 0x148),
			0x14C, INREG32(DISPSYS_MUTEX_BASE + 0x14C),
			0x150, INREG32(DISPSYS_MUTEX_BASE + 0x150),
			0x154, INREG32(DISPSYS_MUTEX_BASE + 0x154));
		DDPDUMP("MUTEX: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x160, INREG32(DISPSYS_MUTEX_BASE + 0x160),
			0x164, INREG32(DISPSYS_MUTEX_BASE + 0x164),
			0x168, INREG32(DISPSYS_MUTEX_BASE + 0x168),
			0x16C, INREG32(DISPSYS_MUTEX_BASE + 0x16C));
		DDPDUMP("MUTEX: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x170, INREG32(DISPSYS_MUTEX_BASE + 0x170),
			0x174, INREG32(DISPSYS_MUTEX_BASE + 0x174),
			0x180, INREG32(DISPSYS_MUTEX_BASE + 0x180),
			0x184, INREG32(DISPSYS_MUTEX_BASE + 0x184));
		DDPDUMP("MUTEX: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x188, INREG32(DISPSYS_MUTEX_BASE + 0x188),
			0x18C, INREG32(DISPSYS_MUTEX_BASE + 0x18C),
			0x190, INREG32(DISPSYS_MUTEX_BASE + 0x190),
			0x194, INREG32(DISPSYS_MUTEX_BASE + 0x194));
		DDPDUMP("MUTEX: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x1A0, INREG32(DISPSYS_MUTEX_BASE + 0x1A0),
			0x1A4, INREG32(DISPSYS_MUTEX_BASE + 0x1A4),
			0x1A8, INREG32(DISPSYS_MUTEX_BASE + 0x1A8),
			0x1AC, INREG32(DISPSYS_MUTEX_BASE + 0x1AC));
		DDPDUMP("MUTEX: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x1B0, INREG32(DISPSYS_MUTEX_BASE + 0x1B0),
			0x1B4, INREG32(DISPSYS_MUTEX_BASE + 0x1B4),
			0x1C0, INREG32(DISPSYS_MUTEX_BASE + 0x1C0),
			0x1C4, INREG32(DISPSYS_MUTEX_BASE + 0x1C4));
		DDPDUMP("MUTEX: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x1C8, INREG32(DISPSYS_MUTEX_BASE + 0x1C8),
			0x1CC, INREG32(DISPSYS_MUTEX_BASE + 0x1CC),
			0x1D0, INREG32(DISPSYS_MUTEX_BASE + 0x1D0),
			0x1D4, INREG32(DISPSYS_MUTEX_BASE + 0x1D4));
		DDPDUMP("MUTEX: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x1E0, INREG32(DISPSYS_MUTEX_BASE + 0x1E0),
			0x1E4, INREG32(DISPSYS_MUTEX_BASE + 0x1E4),
			0x1E8, INREG32(DISPSYS_MUTEX_BASE + 0x1E8),
			0x1EC, INREG32(DISPSYS_MUTEX_BASE + 0x1EC));
		DDPDUMP("MUTEX: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x1F0, INREG32(DISPSYS_MUTEX_BASE + 0x1F0),
			0x1F4, INREG32(DISPSYS_MUTEX_BASE + 0x1F4),
			0x200, INREG32(DISPSYS_MUTEX_BASE + 0x200),
			0x204, INREG32(DISPSYS_MUTEX_BASE + 0x204));
		DDPDUMP("MUTEX: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x208, INREG32(DISPSYS_MUTEX_BASE + 0x208),
			0x20C, INREG32(DISPSYS_MUTEX_BASE + 0x20C),
			0x210, INREG32(DISPSYS_MUTEX_BASE + 0x210),
			0x214, INREG32(DISPSYS_MUTEX_BASE + 0x214));
		DDPDUMP("MUTEX: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x300, INREG32(DISPSYS_MUTEX_BASE + 0x300),
			0x304, INREG32(DISPSYS_MUTEX_BASE + 0x304),
			0x30C, INREG32(DISPSYS_MUTEX_BASE + 0x30C));
		DDPDUMP("-- END: DISP MUTEX REGS --\n");
	} else {
		DDPDUMP("==DISP MUTEX REGS==\n");
		DDPDUMP("(0x000)M_INTEN   =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX_INTEN));
		DDPDUMP("(0x004)M_INTSTA  =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX_INTSTA));
		DDPDUMP("(0x020)M0_EN     =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX0_EN));
		/*DDPDUMP("(0x024)M0_GET     =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX0_GET));*/
		DDPDUMP("(0x028)M0_RST    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX0_RST));
		DDPDUMP("(0x02c)M0_SOF    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX0_SOF));
		DDPDUMP("(0x030)M0_MOD0    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX0_MOD0));
		/*DDPDUMP("(0x034)M0_MOD1    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX0_MOD1));*/
		DDPDUMP("(0x040)M1_EN     =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX1_EN));
		/*DDPDUMP("(0x044)M1_GET     =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX1_GET));*/
		DDPDUMP("(0x048)M1_RST    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX1_RST));
		DDPDUMP("(0x04c)M1_SOF    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX1_SOF));
		DDPDUMP("(0x050)M1_MOD0    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX1_MOD0));
		/*DDPDUMP("(0x054)M1_MOD1    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX1_MOD1));*/
		DDPDUMP("(0x060)M2_EN     =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX2_EN));
		/*DDPDUMP("(0x064)M2_GET     =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX2_GET));*/
		DDPDUMP("(0x068)M2_RST    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX2_RST));
		DDPDUMP("(0x06c)M2_SOF    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX2_SOF));
		DDPDUMP("(0x070)M2_MOD0    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX2_MOD0));
		/*DDPDUMP("(0x074)M2_MOD1    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX2_MOD1));*/
		DDPDUMP("(0x080)M3_EN     =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX3_EN));
		/*DDPDUMP("(0x084)M3_GET     =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX3_GET));*/
		DDPDUMP("(0x088)M3_RST    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX3_RST));
		DDPDUMP("(0x08C)M3_SOF    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX3_SOF));
		DDPDUMP("(0x090)M3_MOD0    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX3_MOD0));
		/*DDPDUMP("(0x090)M3_MOD1    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX3_MOD1));*/
		DDPDUMP("(0x0a0)M4_EN     =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX4_EN));
		/*DDPDUMP("(0x0a4)M4_GET     =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX4_GET));*/
		DDPDUMP("(0x0a8)M4_RST    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX4_RST));
		DDPDUMP("(0x0ac)M4_SOF    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX4_SOF));
		DDPDUMP("(0x0b0)M4_MOD0    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX4_MOD0));
		/*DDPDUMP("(0x0b4)M4_MOD1    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX4_MOD1));*/
		DDPDUMP("(0x0c0)M5_EN     =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX5_EN));
		/*DDPDUMP("(0x0c4)M5_GET     =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX5_GET));*/
		DDPDUMP("(0x0c8)M5_RST    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX5_RST));
		DDPDUMP("(0x0cc)M5_SOF    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX5_SOF));
		DDPDUMP("(0x0d0)M5_MOD0    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX5_MOD0));
/*		DDPDUMP("(0x0d4)M5_MOD1    =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX5_MOD1));
		DDPDUMP("(0x300)DISP_REG_CONFIG_MUTEX_DUMMY0 =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX_DUMMY0));
		DDPDUMP("(0x304)DISP_REG_CONFIG_MUTEX_DUMMY1 =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX_DUMMY1));
*/
		DDPDUMP("(0x30c)DEBUG_OUT_SEL =0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DEBUG_OUT_SEL));
	}

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
		if (DISP_REG_GET(DISP_REG_CONFIG_MUTEX_MOD0(i)) == 0)
			continue;

		val = DISP_REG_GET(DISP_REG_CONFIG_MUTEX_SOF(i));
		len = sprintf(p, "MUTEX%d :SOF=%s,EOF=%s,WAIT=%d,module=(",
			      i,
			      ddp_get_mutex_sof_name(REG_FLD_VAL_GET(SOF_FLD_MUTEX0_SOF, val)),
			      ddp_get_mutex_sof_name(REG_FLD_VAL_GET(SOF_FLD_MUTEX0_EOF, val)),
				REG_FLD_VAL_GET(SOF_FLD_MUTEX0_SOF_WAIT, val));

		p += len;
		for (j = 0; j < 32; j++) {
			unsigned int regval = DISP_REG_GET(DISP_REG_CONFIG_MUTEX_MOD0(i));

			if ((regval & (1 << j))) {
				len = sprintf(p, "%s,", ddp_get_mutex_module0_name(j));
				p += len;
			}
		}
/*
		for (j = 0; j < 32; j++) {
			unsigned int regval = DISP_REG_GET(DISP_REG_CONFIG_MUTEX_MOD1(i));

			if ((regval & (1 << j))) {
				len = sprintf(p, "%s,", ddp_get_mutex_module1_name(j));
				p += len;
			}
		}
*/
		DDPDUMP("%s)\n", mutex_module);
	}
}

static void mmsys_config_dump_reg(void)
{
	if (disp_helper_get_option(DISP_OPT_REG_PARSER_RAW_DUMP)) {
		DDPDUMP("== START: DISP MMSYS_CONFIG REGS ==\n");
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x000, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x000),
			0x004, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x004),
			0x00C, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x00C),
			0x048, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x048));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x0F0, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x0F0),
			0x0F4, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x0F4),
			0x0F8, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x0F8),
			0x100, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x100));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x104, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x104),
			0x108, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x108),
			0x110, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x110),
			0x114, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x114));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x118, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x118),
			0x120, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x120),
			0x124, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x124),
			0x128, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x128));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x130, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x130),
			0x134, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x134),
			0x138, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x138),
			0x140, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x140));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x144, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x144),
			0x150, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x150),
			0x168, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x168),
			0x16C, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x16C));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x170, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x170),
			0x174, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x174),
			0x178, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x178),
			0x17C, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x17C));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x180, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x180),
			0x184, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x184),
			0x190, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x190),
			0x200, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x200));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x204, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x204),
			0x208, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x208),
			0x20C, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x20C),
			0x210, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x210));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x214, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x214),
			0x218, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x218),
			0x220, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x220),
			0x224, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x224));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x228, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x228),
			0x22C, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x22C),
			0x230, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x230),
			0x234, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x234));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x238, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x238),
			0x800, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x800),
			0x804, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x804),
			0x808, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x808));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x80C, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x80C),
			0x810, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x810),
			0x814, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x814),
			0x818, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x818));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x81C, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x81C),
			0x820, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x820),
			0x824, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x824),
			0x828, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x828));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x82C, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x82C),
			0x830, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x830),
			0x834, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x834),
			0x838, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x838));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x83C, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x83C),
			0x840, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x840),
			0x844, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x844),
			0x848, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x848));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x84C, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x84C),
			0x850, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x850),
			0x854, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x854),
			0x858, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x858));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x85C, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x85C),
			0x860, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x860),
			0x864, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x864),
			0x868, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x868));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x86C, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x86C),
			0x870, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x870),
			0x874, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x874),
			0x878, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x878));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x87C, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x87C),
			0x880, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x880),
			0x884, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x884),
			0x888, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x888));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x88C, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x88C),
			0x890, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x890),
			0x894, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x894),
			0x898, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x898));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x89C, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x89C),
			0x8A0, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x8A0),
			0x8A4, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x8A4),
			0x8A8, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x8A8));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x8AC, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x8AC),
			0x8B0, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x8B0),
			0x8B4, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x8B4),
			0x8B8, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x8B8));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x8BC, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x8BC),
			0x8C0, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x8C0),
			0x8C4, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x8C4),
			0x8C8, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x8C8));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x8CC, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x8CC),
			0x8D0, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x8D0),
			0x8D4, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x8D4),
			0x8D8, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x8D8));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x8DC, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x8DC),
			0x8E0, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x8E0),
			0x8E4, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x8E4),
			0x8E8, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0x8E8));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0xF00, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xF00),
			0xF04, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xF04),
			0xF08, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xF08),
			0xF0C, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xF0C));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0xF10, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xF10),
			0xF14, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xF14),
			0xF18, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xF18),
			0xF1C, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xF1C));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0xF20, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xF20),
			0xF24, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xF24),
			0xF28, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xF28),
			0xF2C, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xF2C));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0xF30, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xF30),
			0xF34, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xF34),
			0xF38, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xF38),
			0xF3C, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xF3C));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0xF40, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xF40),
			0xF44, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xF44),
			0xF48, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xF48),
			0xF4C, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xF4C));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0xF50, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xF50),
			0xF54, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xF54),
			0xF58, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xF58),
			0xF5C, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xF5C));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0xF60, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xF60),
			0xF64, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xF64),
			0xF80, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xF80),
			0xF84, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xF84));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0xF88, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xF88),
			0xF8C, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xF8C),
			0xF90, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xF90),
			0xF94, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xF94));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0xF98, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xF98),
			0XF9C, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0XF9C),
			0xFA0, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xFA0),
			0xFA4, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xFA4));
		DDPDUMP("MMSYS_CONFIG: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0xFA8, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xFA8),
			0xFAC, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xFAC),
			0xFB0, INREG32(DDP_REG_BASE_MMSYS_CONFIG + 0xFB0));
		DDPDUMP("-- END: DISP MMSYS_CONFIG REGS --\n");
	} else {
		DDPDUMP("== DISP MMSYS_Config REGS ==\n");
		DDPDUMP("OVL0_MOUT_EN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_OVL0_MOUT_EN));
		DDPDUMP("OVL0_PQ_MOUT_EN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_OVL0_PQ_MOUT_EN));
		DDPDUMP("OVL1_PQ_MOUT_EN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_OVL1_PQ_MOUT_EN));
		DDPDUMP("DITHER_MOUT_EN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_DITHER_MOUT_EN));
		DDPDUMP("COLOR0_SEL_IN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_COLOR0_SEL_IN));
		DDPDUMP("WDMA0_SEL_IN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_WDMA0_SEL_IN));
		DDPDUMP("DSI0_SEL_IN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DSI0_SEL_IN));
		DDPDUMP("DPI_SEL_IN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DPI0_SEL_IN));
		DDPDUMP("OVL0_SEL_IN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_OVL0_SEL_IN));
		DDPDUMP("RDMA0_SOUT_SEL_IN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_RDMA0_SOUT_SEL_IN));
		DDPDUMP("RDMA1_SOUT_SEL_IN=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_DISP_RDMA1_SOUT_SEL_IN));

		DDPDUMP("(0x0F0)MM_MISC=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_MISC));
		DDPDUMP("(0x100)MM_CG_CON0=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON0));
		DDPDUMP("(0x110)MM_CG_CON1=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON1));
		DDPDUMP("(0x120)MM_HW_DCM_DIS0=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_HW_DCM_DIS0));
		DDPDUMP("(0x140)MM_SW0_RST_B=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_SW0_RST_B));
		DDPDUMP("(0x144)MM_SW1_RST_B=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_SW1_RST_B));
		DDPDUMP("(0x150)MM_LCM_RST_B=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_LCM_RST_B));
		DDPDUMP("(0x880)MM_DBG_OUT_SEL=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_DEBUG_OUT_SEL));
		DDPDUMP("(0x890)MM_DUMMY0=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_DUMMY0));
		DDPDUMP("(0x894)MM_DUMMY1=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_DUMMY1));
		DDPDUMP("(0x898)MM_DUMMY2=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_DUMMY2));
		DDPDUMP("(0x89C)MM_DUMMY3=0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_DUMMY3));

		DDPDUMP("== DISP LARB REGS ==\n");
		DDPDUMP("Larb0 port0:%d\n", DISP_REG_GET(DISPSYS_SMI_LARB0_BASE + 0x380));
		DDPDUMP("Larb0 port1:%d\n", DISP_REG_GET(DISPSYS_SMI_LARB0_BASE + 0x384));
		DDPDUMP("Larb0 MMU EN:0x%x\n", DISP_REG_GET(DISPSYS_SMI_LARB0_BASE+0xf80));
	}

}
#if 0
------ clock:
Before power on mmsys :
CLK_CFG_0_CLR(address is 0x10000048) = 0x80000000(bit 31).
Before using DISP_PWM0 or DISP_PWM1 :
CLK_CFG_1_CLR(address is 0x10000058) = 0x80(bit 7).
Before using DPI pixel clock :
CLK_CFG_6_CLR(address is 0x100000A8) = 0x80(bit 7).

Only need to enable the corresponding bits of MMSYS_CG_CON0 and MMSYS_CG_CON1 for the modules :
smi_common, larb0, mdp_crop, fake_eng, mutex_32k, pwm0, pwm1, dsi0, dsi1, dpi.
Other bits could keep 1. Suggest to keep smi_common and larb0 always clock on.

--------valid & ready
example :
ovl0->ovl0_mout_ready = 1 means engines after ovl_mout are ready for receiving data
	ovl0_mout_ready = 0 means ovl0_mout can not receive data, maybe ovl0_mout or after engines config error
ovl0->ovl0_mout_valid = 1 means engines before ovl0_mout is OK,
	ovl0_mout_valid = 0 means ovl can not transfer data to ovl0_mout, means ovl0 or before engines are not ready.
#endif

static void mmsys_config_dump_analysis(void)
{
	unsigned int i = 0, j;
	unsigned int reg = 0;
	char clock_on[512] = { '\0' };
	char *pos = NULL;
	const char *name;
	/* int len = 0; */
	unsigned int valid[3], ready[3];
	unsigned int greq;

	valid[0] = DISP_REG_GET(DISP_REG_CONFIG_DISP_DL_VALID_0);
	valid[1] = DISP_REG_GET(DISP_REG_CONFIG_DISP_DL_VALID_1);
	valid[2] = DISP_REG_GET(DISP_REG_CONFIG_DISP_DL_VALID_2);
	ready[0] = DISP_REG_GET(DISP_REG_CONFIG_DISP_DL_READY_0);
	ready[1] = DISP_REG_GET(DISP_REG_CONFIG_DISP_DL_READY_1);
	ready[2] = DISP_REG_GET(DISP_REG_CONFIG_DISP_DL_READY_2);
	greq = DISP_REG_GET(DISP_REG_CONFIG_SMI_LARB0_GREQ);

	DDPDUMP("== DISP MMSYS_CONFIG ANALYSIS ==\n");
	DDPDUMP("mmsys CG_CON0=0x%x, CG_CON1=0x%x\n",
		DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON0),
		DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON1));

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

	DDPDUMP("valid0=0x%x,valid1=0x%x,valid2=0x%x,ready0=0x%x,ready1=0x%x,ready2=0x%x,greq=0%x\n",
		valid[0], valid[1], valid[2], ready[0], ready[1], ready[2], greq);

	for (j = 0; j < ARRAY_SIZE(valid); j++) {
		for (i = 0; i < 32; i++) {
			name = ddp_signal[j][i];
			if (!name)
				continue;

			pos = clock_on;

			if ((valid[j] & (1 << i)))
				pos += sprintf(pos, "%s,", "v");
			else
				pos += sprintf(pos, "%s,", "n");

			if ((ready[j] & (1 << i)))
				pos += sprintf(pos, "%s", "r");
			else
				pos += sprintf(pos, "%s", "n");

			pos += sprintf(pos, ": %s", name);

			DDPDUMP("%s\n", clock_on);
		}
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
	DDPDUMP("gamma: en=%d, w=%d, h=%d, in_p_cnt=%d, in_l_cnt=%d, out_p_cnt=%d, out_l_cnt=%d, relay=%d, stall=%d\n",
		DISP_REG_GET(DISP_REG_GAMMA_EN),
		(DISP_REG_GET(DISP_REG_GAMMA_SIZE) >> 16) & 0x1fff,
		DISP_REG_GET(DISP_REG_GAMMA_SIZE) & 0x1fff,
		DISP_REG_GET(DISP_REG_GAMMA_INPUT_COUNT) & 0x1fff,
		(DISP_REG_GET(DISP_REG_GAMMA_INPUT_COUNT) >> 16) & 0x1fff,
		DISP_REG_GET(DISP_REG_GAMMA_OUTPUT_COUNT) & 0x1fff,
		(DISP_REG_GET(DISP_REG_GAMMA_OUTPUT_COUNT) >> 16) & 0x1fff,
		DISP_REG_GET(DISP_REG_GAMMA_CFG) & 0x1,
		(DISP_REG_GET(DISP_REG_GAMMA_CFG) >> 8) & 0x1);
}

static void color_dump_reg(enum DISP_MODULE_ENUM module)
{
	int index = 0;

	if (module == DISP_MODULE_COLOR0) {
		index = 0;
	} else if (module == DISP_MODULE_COLOR1)	{
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

}

static void color_dump_analysis(enum DISP_MODULE_ENUM module)
{
	int index = 0;

	if (module == DISP_MODULE_COLOR0) {
		index = 0;
	} else if (module == DISP_MODULE_COLOR1) {
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

static void pwm_dump_reg(enum DISP_MODULE_ENUM module)
{
	int index = 0;
	unsigned long reg_base = 0;

	if (module == DISP_MODULE_PWM0) {
		index = 0;
		reg_base = DISPSYS_PWM0_BASE;
	} else {
		index = 1;
		reg_base = DISPSYS_PWM0_BASE;
	}
	if (disp_helper_get_option(DISP_OPT_REG_PARSER_RAW_DUMP)) {
		DDPDUMP("== START: DISP PWM0 REGS ==\n");
		DDPDUMP("PWM0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x0, INREG32(reg_base + 0x0),
			0x4, INREG32(reg_base + 0x4),
			0x8, INREG32(reg_base + 0x8),
			0x10, INREG32(reg_base + 0x10));
		DDPDUMP("PWM0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x14, INREG32(reg_base + 0x14),
			0x18, INREG32(reg_base + 0x18),
			0x1C, INREG32(reg_base + 0x1C),
			0x20, INREG32(reg_base + 0x20));
		DDPDUMP("PWM0: 0x%04x=0x%08x\n",
			0x30, INREG32(reg_base + 0x30));
		DDPDUMP("-- END: DISP PWM0 REGS --\n");
	} else {
		DDPDUMP("== DISP PWM%d REGS ==\n", index);
		DDPDUMP("(0x000)PWM_EN=0x%x\n", DISP_REG_GET(reg_base + DISP_PWM_EN_OFF));
		DDPDUMP("(0x008)PWM_CON_0=0x%x\n", DISP_REG_GET(reg_base + DISP_PWM_CON_0_OFF));
		DDPDUMP("(0x010)PWM_CON_1=0x%x\n", DISP_REG_GET(reg_base + DISP_PWM_CON_1_OFF));
		DDPDUMP("(0x028)PWM_DEBUG=0x%x\n", DISP_REG_GET(reg_base + 0x28));
	}

}

static void pwm_dump_analysis(enum DISP_MODULE_ENUM module)
{
	int index = 0;
	unsigned int reg_base = 0;

	if (module == DISP_MODULE_PWM0) {
		index = 0;
		reg_base = DISPSYS_PWM0_BASE;
	} else {
		index = 1;
		reg_base = DISPSYS_PWM0_BASE;
	}
	DDPDUMP("== DISP PWM%d ANALYSIS ==\n", index);
#if 0 /* TODO: clk reg?? */
	DDPDUMP("pwm clock=%d\n", (DISP_REG_GET(DISP_REG_CLK_CFG_1_CLR) >> 7) & 0x1);
#endif

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

}

static void od_dump_analysis(void)
{
	DDPDUMP("== DISP OD ANALYSIS ==\n");
	DDPDUMP("od: w=%d, h=%d, bypass=%d\n",
		(DISP_REG_GET(DISP_REG_OD_SIZE) >> 16) & 0xffff,
		DISP_REG_GET(DISP_REG_OD_SIZE) & 0xffff, DISP_REG_GET(DISP_REG_OD_CFG) & 0x1);

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
	     DISP_REG_GET(DISP_REG_CCORR_OUT_CNT) & 0x1fff,
	     (DISP_REG_GET(DISP_REG_CCORR_OUT_CNT) >> 16) & 0x1fff);
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
	DDPDUMP("dither: en=%d, config=%d, w=%d, h=%d, in_p_cnt=%d, in_l_cnt=%d, out_p_cnt=%d, out_l_cnt=%d\n",
		 DISP_REG_GET(DISPSYS_DITHER0_BASE + 0x000), DISP_REG_GET(DISPSYS_DITHER0_BASE + 0x020),
		 (DISP_REG_GET(DISP_REG_DITHER_SIZE) >> 16) & 0x1fff,
		 DISP_REG_GET(DISP_REG_DITHER_SIZE) & 0x1fff,
		 DISP_REG_GET(DISP_REG_DITHER_IN_CNT) & 0x1fff,
		 (DISP_REG_GET(DISP_REG_DITHER_IN_CNT) >> 16) & 0x1fff,
		 DISP_REG_GET(DISP_REG_DITHER_OUT_CNT) & 0x1fff,
		 (DISP_REG_GET(DISP_REG_DITHER_OUT_CNT) >> 16) & 0x1fff);
	DDPDUMP("relay=%d\n", DISP_REG_GET(DISP_REG_DITHER_CFG) & 0x1);
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
#if 0

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
#endif
}

static void dsi_dump_reg(enum DISP_MODULE_ENUM module)
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
	if (disp_helper_get_option(DISP_OPT_REG_PARSER_RAW_DUMP)) {
		DDPDUMP("== START: DISP SPLIT REGS ==\n");
		DDPDUMP("SPLIT: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x00, INREG32(DDP_REG_BASE_DISP_SPLIT0 + 0x00),
			0x04, INREG32(DDP_REG_BASE_DISP_SPLIT0 + 0x04),
			0x20, INREG32(DDP_REG_BASE_DISP_SPLIT0 + 0x20),
			0x60, INREG32(DDP_REG_BASE_DISP_SPLIT0 + 0x60));
		DDPDUMP("SPLIT: 0x%04x=0x%08x\n",
			0x80, INREG32(DDP_REG_BASE_DISP_SPLIT0 + 0x80));
		DDPDUMP("-- END: DISP SPLIT REGS --\n");
	} else {
		DDPDUMP("== DISP SPLIT0 REGS ==\n");
		DDPDUMP("(0x000)S_ENABLE=0x%x\n", DISP_REG_GET(DISP_REG_SPLIT_ENABLE));
		DDPDUMP("(0x004)S_SW_RST=0x%x\n", DISP_REG_GET(DISP_REG_SPLIT_SW_RESET));
		DDPDUMP("(0x008)S_DEBUG=0x%x\n", DISP_REG_GET(DISP_REG_SPLIT_DEBUG));
	}
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

int ddp_dump_reg(enum DISP_MODULE_ENUM module)
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
	case DISP_MODULE_OVL0_2L:
	case DISP_MODULE_OVL1_2L:
		ovl_dump_reg(module);
		break;
	case DISP_MODULE_GAMMA0:
		gamma_dump_reg();
		break;
	case DISP_MODULE_CONFIG:
		mmsys_config_dump_reg();
		break;
	case DISP_MODULE_MUTEX:
		mutex_dump_reg();
		break;
	case DISP_MODULE_SPLIT0:
		split_dump_regs();
		break;
	case DISP_MODULE_COLOR0:
	case DISP_MODULE_COLOR1:
		color_dump_reg(module);
		break;
	case DISP_MODULE_AAL0:
		aal_dump_reg();
		break;
	case DISP_MODULE_PWM0:
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
	case DISP_MODULE_CCORR0:
		ccorr_dump_reg();
		break;
	case DISP_MODULE_DITHER0:
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

int ddp_dump_analysis(enum DISP_MODULE_ENUM module)
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
	case DISP_MODULE_OVL0_2L:
	case DISP_MODULE_OVL1_2L:
		ovl_dump_analysis(module);
		break;
	case DISP_MODULE_GAMMA0:
		gamma_dump_analysis();
		break;
	case DISP_MODULE_CONFIG:
		mmsys_config_dump_analysis();
		break;
	case DISP_MODULE_MUTEX:
		mutex_dump_analysis();
		break;
	case DISP_MODULE_SPLIT0:
		split_dump_analysis();
		break;
	case DISP_MODULE_COLOR0:
	case DISP_MODULE_COLOR1:
		color_dump_analysis(module);
		break;
	case DISP_MODULE_AAL0:
		aal_dump_analysis();
		break;
	case DISP_MODULE_OD:
		od_dump_analysis();
		break;
	case DISP_MODULE_PWM0:
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
	case DISP_MODULE_CCORR0:
		ccorr_dump_analyze();
		break;
	case DISP_MODULE_DITHER0:
		dither_dump_analyze();
		break;
	default:
		DDPDUMP("no dump_analysis for module %s(%d)\n", ddp_get_module_name(module),
			module);
	}
	return 0;
}

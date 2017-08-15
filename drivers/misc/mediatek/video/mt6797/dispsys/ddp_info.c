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

#define LOG_TAG "INFO"
#include "ddp_info.h"
#include "ddp_debug.h"
#include "ddp_log.h"

char *ddp_get_module_name(DISP_MODULE_ENUM module)
{
	switch (module) {
	case DISP_MODULE_AAL:
		return "aal";
	case DISP_MODULE_COLOR0:
		return "color0";
	case DISP_MODULE_COLOR1:
		return "color1";
	case DISP_MODULE_RDMA0:
		return "rdma0";
	case DISP_MODULE_RDMA1:
		return "rdma1";
	case DISP_MODULE_RDMA2:
		return "rdma2";
	case DISP_MODULE_WDMA0:
		return "wdma0";
	case DISP_MODULE_OVL0:
		return "ovl0";
	case DISP_MODULE_OVL1:
		return "ovl1";
	case DISP_MODULE_WDMA1:
		return "wdma1";
	case DISP_MODULE_OVL0_2L:
		return "ovl0_2l";
	case DISP_MODULE_OVL1_2L:
		return "ovl1_2l";
	case DISP_MODULE_GAMMA:
		return "gamma";
	case DISP_MODULE_PWM0:
		return "pwm0";
	case DISP_MODULE_PWM1:
		return "pwm1";
	case DISP_MODULE_OD:
		return "od";
	case DISP_MODULE_MERGE:
		return "merge";
	case DISP_MODULE_SPLIT0:
		return "split0";
	case DISP_MODULE_SPLIT1:
		return "split1";
	case DISP_MODULE_DSI0:
		return "dsi0";
	case DISP_MODULE_DSI1:
		return "dsi1";
	case DISP_MODULE_DSIDUAL:
		return "dsidual";
	case DISP_MODULE_DPI:
		return "dpi";
	case DISP_MODULE_SMI:
		return "smi";
	case DISP_MODULE_CONFIG:
		return "config";
	case DISP_MODULE_CMDQ:
		return "cmdq";
	case DISP_MODULE_MUTEX:
		return "mutex";
	case DISP_MODULE_CCORR:
		return "ccorr";
	case DISP_MODULE_DITHER:
		return "dither";
	case DISP_MODULE_SMI_LARB0:
		return "smi_larb0";
	case DISP_MODULE_SMI_LARB5:
		return "smi_larb5";
	case DISP_MODULE_SMI_COMMON:
		return "smi_common";
	case DISP_MODULE_UFOE:
		return "ufoe";
	case DISP_MODULE_OVL0_VIRTUAL:
		return "ovl0_virtual";
	case DISP_MODULE_MIPI0:
		return "mipi0";
	case DISP_MODULE_MIPI1:
		return "mipi1";
	case DISP_MODULE_DSC:
		return "dsc";
	case DISP_PATH0:
		return "path0";

	default:
		DDPMSG("invalid module id=%d", module);
		return "unknown";
	}
}

DISP_MODULE_ENUM ddp_get_reg_module(DISP_REG_ENUM reg_module)
{
	switch (reg_module) {
	case DISP_REG_CONFIG:
		return DISP_MODULE_CONFIG;
	case DISP_REG_OVL0:
		return DISP_MODULE_OVL0;
	case DISP_REG_OVL1:
		return DISP_MODULE_OVL1;
	case DISP_REG_RDMA0:
		return DISP_MODULE_RDMA0;
	case DISP_REG_RDMA1:
		return DISP_MODULE_RDMA1;
	case DISP_REG_WDMA0:
		return DISP_MODULE_WDMA0;
	case DISP_REG_COLOR:
		return DISP_MODULE_COLOR0;
	case DISP_REG_CCORR:
		return DISP_MODULE_CCORR;
	case DISP_REG_AAL:
		return DISP_MODULE_AAL;
	case DISP_REG_GAMMA:
		return DISP_MODULE_GAMMA;
	case DISP_REG_DITHER:
		return DISP_MODULE_DITHER;
	case DISP_REG_UFOE:
		return DISP_MODULE_UFOE;
	case DISP_REG_DSC:
		return DISP_MODULE_DSC;
	case DISP_REG_SPLIT0:
		return DISP_MODULE_SPLIT0;
	case DISP_REG_DSI0:
		return DISP_MODULE_DSI0;
	case DISP_REG_DSI1:
		return DISP_MODULE_DSI1;
	case DISP_REG_DPI0:
		return DISP_MODULE_DPI;
	case DISP_REG_PWM:
		return DISP_MODULE_PWM0;
	case DISP_REG_MUTEX:
		return DISP_MODULE_MUTEX;
	case DISP_REG_SMI_LARB0:
		return DISP_MODULE_SMI_LARB0;
	case DISP_REG_SMI_LARB5:
		return DISP_MODULE_SMI_LARB5;
	case DISP_REG_SMI_COMMON:
		return DISP_MODULE_SMI_COMMON;
	case DISP_REG_WDMA1:
		return DISP_MODULE_WDMA1;
	case DISP_REG_OVL0_2L:
		return DISP_MODULE_OVL0_2L;
	case DISP_REG_OVL1_2L:
		return DISP_MODULE_OVL1_2L;
	case DISP_REG_MIPI0:
		return DISP_MODULE_MIPI0;
	case DISP_REG_MIPI1:
		return DISP_MODULE_MIPI1;
	case DISP_REG_OD:
		return DISP_MODULE_OD;
	default:
		DDPERR("%s: invalid reg module id=%d\n", __func__, reg_module);
		WARN(1, "%s: invalid reg module id=%d\n", __func__, reg_module);
		break;
	}

	return DISP_MODULE_UNKNOWN;
}

char *ddp_get_reg_module_name(DISP_REG_ENUM reg_module)
{
	return ddp_get_module_name(ddp_get_reg_module(reg_module));
}

int ddp_get_module_max_irq_bit(DISP_MODULE_ENUM module)
{
	switch (module) {
	case DISP_MODULE_AAL:
		return 1;
	case DISP_MODULE_COLOR0:
		return 2;
	case DISP_MODULE_COLOR1:
		return 2;
	case DISP_MODULE_RDMA0:
		return 5;
	case DISP_MODULE_RDMA1:
		return 5;
	case DISP_MODULE_RDMA2:
		return 5;
	case DISP_MODULE_WDMA0:
		return 1;
	case DISP_MODULE_OVL0:
		return 3;
	case DISP_MODULE_OVL1:
		return 3;
	case DISP_MODULE_WDMA1:
		return 1;
	case DISP_MODULE_OVL0_2L:
		return 3;
	case DISP_MODULE_OVL1_2L:
		return 3;
	case DISP_MODULE_GAMMA:
		return 0;
	case DISP_MODULE_PWM0:
		return 0;
	case DISP_MODULE_PWM1:
		return 0;
	case DISP_MODULE_OD:
		return 0;
	case DISP_MODULE_MERGE:
		return 0;
	case DISP_MODULE_SPLIT0:
		return 0;
	case DISP_MODULE_SPLIT1:
		return 0;
	case DISP_MODULE_DSI0:
		return 6;
	case DISP_MODULE_DSI1:
		return 6;
	case DISP_MODULE_DSIDUAL:
		return 6;
	case DISP_MODULE_DPI:
		return 2;
	case DISP_MODULE_SMI:
		return 0;
	case DISP_MODULE_CONFIG:
		return 0;
	case DISP_MODULE_CMDQ:
		return 0;
	case DISP_MODULE_MUTEX:
		return 14;
	case DISP_MODULE_CCORR:
		return 0;
	case DISP_MODULE_DITHER:
		return 0;
	default:
		DDPMSG("invalid module id=%d", module);
	}
	return 0;
}

unsigned int ddp_module_to_idx(int module)
{
	unsigned int id = 0;
	switch (module) {
	case DISP_MODULE_AAL:
	case DISP_MODULE_COLOR0:
	case DISP_MODULE_RDMA0:
	case DISP_MODULE_WDMA0:
	case DISP_MODULE_OVL0:
	case DISP_MODULE_GAMMA:
	case DISP_MODULE_PWM0:
	case DISP_MODULE_OD:
	case DISP_MODULE_SPLIT0:
	case DISP_MODULE_DSI0:
	case DISP_MODULE_DPI:
	case DISP_MODULE_DITHER:
	case DISP_MODULE_CCORR:
		id = 0;
		break;

	case DISP_MODULE_COLOR1:
	case DISP_MODULE_RDMA1:
	case DISP_MODULE_PWM1:
	case DISP_MODULE_SPLIT1:
	case DISP_MODULE_DSI1:
	case DISP_MODULE_OVL1:
	case DISP_MODULE_WDMA1:
		id = 1;
		break;
	case DISP_MODULE_RDMA2:
	case DISP_MODULE_DSIDUAL:
		id = 2;
		break;
	default:
		DDPERR("ddp_module_to_idx, module=0x%x\n", module);
	}

	return id;
}

DDP_MODULE_DRIVER *ddp_modules_driver[DISP_MODULE_NUM] = {
	&ddp_driver_ovl,	/* DISP_MODULE_OVL0 = 0, */
	&ddp_driver_ovl,	/* DISP_MODULE_OVL1  , */
	&ddp_driver_ovl,	/* DISP_MODULE_OVL0_2L  , */
	&ddp_driver_ovl,	/* DISP_MODULE_OVL1_2L  , */
	0,			/* DISP_MODULE_OVL0_VIRTUAL */

	&ddp_driver_rdma,	/* DISP_MODULE_RDMA0 , */
	&ddp_driver_rdma,	/* DISP_MODULE_RDMA1 , */
	&ddp_driver_wdma,	/* DISP_MODULE_WDMA0 , */
	&ddp_driver_color,	/* DISP_MODULE_COLOR0, */
	&ddp_driver_ccorr,	/* DISP_MODULE_CCORR , */

	&ddp_driver_aal,	/* DISP_MODULE_AAL   , */
	&ddp_driver_gamma,	/* DISP_MODULE_GAMMA , */
	&ddp_driver_dither,	/* DISP_MODULE_DITHER, */
	0,			/* DISP_PATH0 */
	&ddp_driver_ufoe,	/* DISP_MODULE_UFOE  , //10 */

	&ddp_driver_dsc,	/* DISP_MODULE_DSC */
	&ddp_driver_pwm,	/* DISP_MODULE_PWM0   , */
	&ddp_driver_wdma,	/* DISP_MODULE_WDMA1 , */
#ifndef DISP_NO_DPI
	&ddp_driver_dpi,	/* DISP_MODULE_DPI   , */
#else
	0,
#endif
	0,			/* DISP_MODULE_SMI, */

	0,			/* DISP_MODULE_CONFIG, */
	0,			/* DISP_MODULE_CMDQ, */
	0,			/* DISP_MODULE_MUTEX, */
	0,			/* DISP_MODULE_COLOR1, */
	0,			/* DISP_MODULE_RDMA2, */

	0,			/* DISP_MODULE_PWM1, */
	&ddp_driver_od,		/* DISP_MODULE_OD, */
	0,			/* DISP_MODULE_MERGE, */
	&ddp_driver_split,	/* DISP_MODULE_SPLIT0, */
	0,			/* DISP_MODULE_SPLIT1, */

	&ddp_driver_dsi0,	/* DISP_MODULE_DSI0  , */
	&ddp_driver_dsi1,	/* DISP_MODULE_DSI1  , */
	&ddp_driver_dsidual,	/* DISP_MODULE_DSIDUAL, */
	0,			/* DISP_MODULE_SMI_LARB0 , */
	0,			/* DISP_MODULE_SMI_COMMON, */
	0,			/* DISP_MODULE_MIPI0 */
	0,			/* DISP_MODULE_MIPI1 */
	0,			/* DISP_MODULE_UNKNOWN, //20 */
};

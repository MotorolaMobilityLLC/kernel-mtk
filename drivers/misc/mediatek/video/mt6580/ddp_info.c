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
#include "disp_debug.h"
#include "mtkfb_debug.h"
#include "disp_log.h"

char *ddp_get_module_name(DISP_MODULE_ENUM module)
{
	switch (module) {
	case DISP_MODULE_AAL:
		return "aal ";
	case DISP_MODULE_COLOR0:
		return "color0 ";
	case DISP_MODULE_COLOR1:
		return "color1 ";
	case DISP_MODULE_RDMA0:
		return "rdma0 ";
	case DISP_MODULE_RDMA1:
		return "rdma1 ";
	case DISP_MODULE_RDMA2:
		return "rdma2 ";
	case DISP_MODULE_UFOE:
		return "ufoe ";
	case DISP_MODULE_WDMA0:
		return "wdma0 ";
	case DISP_MODULE_OVL0:
		return "ovl0 ";
	case DISP_MODULE_GAMMA:
		return "gamma ";
	case DISP_MODULE_PWM0:
		return "pwm0 ";
	case DISP_MODULE_PWM1:
		return "pwm1 ";
	case DISP_MODULE_OD:
		return "od ";
	case DISP_MODULE_MERGE:
		return "merge ";
	case DISP_MODULE_SPLIT0:
		return "split0 ";
	case DISP_MODULE_SPLIT1:
		return "split1 ";
	case DISP_MODULE_DSI0:
		return "dsi0 ";
	case DISP_MODULE_DSI1:
		return "dsi1 ";
	case DISP_MODULE_DSIDUAL:
		return "dsidual ";
	case DISP_MODULE_DPI:
		return "dpi ";
	case DISP_MODULE_SMI:
		return "smi ";
	case DISP_MODULE_CONFIG:
		return "config ";
	case DISP_MODULE_CMDQ:
		return "cmdq ";
	case DISP_MODULE_MUTEX:
		return "mutex ";
	case DISP_MODULE_CCORR:
		return "ccorr";
	case DISP_MODULE_DITHER:
		return "dither";
	case DISP_MODULE_SMI_LARB0:
		return "smi_larb0";
	case DISP_MODULE_SMI_COMMON:
		return "smi_common";
	default:
		DISPMSG("invalid module id=%d", module);
		return "unknown";
	}
}

char *ddp_get_reg_module_name(DISP_MODULE_ENUM module)
{
	switch (module) {
	case DISP_REG_OVL0:
		return "ovl0";
	case DISP_REG_RDMA0:
		return "rdma0";
	case DISP_REG_RDMA1:
		return "rdma1";
	case DISP_REG_WDMA0:
		return "wdma0";
	case DISP_REG_COLOR:
		return "color";
	case DISP_REG_UFOE:
		return "ufoe";
	case DISP_REG_CCORR:
		return "ccorr";
	case DISP_REG_AAL:
		return "aal";
	case DISP_REG_GAMMA:
		return "gamma";
	case DISP_REG_DITHER:
		return "dither";
	case DISP_REG_PWM:
		return "pwm";
	case DISP_REG_MUTEX:
		return "mutex";
	case DISP_REG_DSI0:
		return "dsi0";
	case DISP_REG_DPI0:
		return "dpi0";
	case DISP_REG_CONFIG:
		return "config";
	case DISP_REG_SMI_LARB0:
		return "smi_larb0";
	case DISP_REG_SMI_COMMON:
		return "smi_common";
	case DISP_REG_MIPI:
		return "mipi";
	default:
		DISPMSG("invalid module id=%d", module);
		return "unknown";
	}
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
		DISPMSG("invalid module id=%d", module);
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
		id = 1;
		break;
	case DISP_MODULE_RDMA2:
	case DISP_MODULE_DSIDUAL:
		id = 2;
		break;
	default:
		DISPERR("ddp_module_to_idx, module=0x%x\n", module);
	}

	return id;
}


DDP_MODULE_DRIVER *ddp_modules_driver[DISP_MODULE_NUM] = {

	&ddp_driver_ovl,	/* DISP_MODULE_OVL0 = 0, */
	&ddp_driver_ovl,	/* DISP_MODULE_OVL1  , */
	&ddp_driver_rdma,	/* DISP_MODULE_RDMA0 , */
	&ddp_driver_rdma,	/* DISP_MODULE_RDMA1 , */
	&ddp_driver_wdma,	/* DISP_MODULE_WDMA0 , */
	&ddp_driver_color,	/* DISP_MODULE_COLOR0, */
	0,			/* DISP_MODULE_CCORR , */
	&ddp_driver_aal,	/* DISP_MODULE_AAL   , */
	&ddp_driver_gamma,	/* DISP_MODULE_GAMMA , */
	&ddp_driver_dither,	/* DISP_MODULE_DITHER, */
	0,			/* DISP_MODULE_UFOE  , //10 */
	&ddp_driver_pwm,	/* DISP_MODULE_PWM0   , */
	&ddp_driver_wdma,	/* DISP_MODULE_WDMA1 , */
	&ddp_driver_dsi0,	/* DISP_MODULE_DSI0  , */
	0,			/* DISP_MODULE_DPI   , */
	0,			/* DISP_MODULE_SMI, */
	0,			/* DISP_MODULE_CONFIG, */
	0,			/* DISP_MODULE_CMDQ, */
	0,			/* DISP_MODULE_MUTEX, */

	0,			/* DISP_MODULE_COLOR1, */
	0,			/* DISP_MODULE_RDMA2, */
	0,			/* DISP_MODULE_PWM1, */
	0,			/* DISP_MODULE_OD, */
	0,			/* DISP_MODULE_MERGE, */
	0,			/* DISP_MODULE_SPLIT0, */
	0,			/* DISP_MODULE_SPLIT1, */
	0,			/* DISP_MODULE_DSI1, */
	0,			/* DISP_MODULE_DSIDUAL, */

	0,			/* DISP_MODULE_SMI_LARB0 , */
	0,			/* DISP_MODULE_SMI_COMMON, */
	0,			/* DISP_MODULE_UNKNOWN, //20 */
};

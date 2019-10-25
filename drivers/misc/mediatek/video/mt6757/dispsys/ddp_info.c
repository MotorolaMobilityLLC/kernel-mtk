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

char *ddp_get_module_name(enum DISP_MODULE_ENUM module)
{
	switch (module) {
	case DISP_MODULE_OVL0:
		return "ovl0 ";
	case DISP_MODULE_OVL1:
		return "ovl1 ";
	case DISP_MODULE_OVL0_2L:
		return "ovl0_2l ";
	case DISP_MODULE_OVL1_2L:
		return "ovl1_2l ";
	case DISP_MODULE_OVL0_VIRTUAL:
		return "ovl0_virt ";
	case DISP_MODULE_RDMA0:
		return "rdma0 ";
	case DISP_MODULE_RDMA1:
		return "rdma1 ";
	case DISP_MODULE_RDMA2:
		return "rdma2 ";
	case DISP_MODULE_WDMA0:
		return "wdma0 ";
	case DISP_MODULE_WDMA1:
		return "wdma1 ";
	case DISP_MODULE_COLOR0:
		return "color0 ";
	case DISP_MODULE_COLOR1:
		return "color1 ";
	case DISP_MODULE_CCORR0:
		return "ccorr0 ";
	case DISP_MODULE_CCORR1:
		return "ccorr1 ";
	case DISP_MODULE_AAL0:
		return "aal0 ";
	case DISP_MODULE_AAL1:
		return "aal1 ";
	case DISP_MODULE_GAMMA0:
		return "gamma0 ";
	case DISP_MODULE_GAMMA1:
		return "gamma1 ";
	case DISP_MODULE_OD:
		return "od ";
	case DISP_MODULE_DITHER0:
		return "dither0 ";
	case DISP_MODULE_DITHER1:
		return "dither1 ";
	case DISP_PATH0:
		return "path0 ";
	case DISP_PATH1:
		return "path1 ";
	case DISP_MODULE_UFOE:
		return "ufoe ";
	case DISP_MODULE_DSC:
		return "dsc ";
	case DISP_MODULE_SPLIT0:
		return "split0 ";
	case DISP_MODULE_DPI:
		return "dpi ";
	case DISP_MODULE_DSI0:
		return "dsi0 ";
	case DISP_MODULE_DSI1:
		return "dsi1 ";
	case DISP_MODULE_DSIDUAL:
		return "dsidual ";
	case DISP_MODULE_PWM0:
		return "pwm0 ";
	case DISP_MODULE_CONFIG:
		return "config ";
	case DISP_MODULE_MUTEX:
		return "mutex ";
	case DISP_MODULE_SMI_COMMON:
		return "smi_common";
	case DISP_MODULE_SMI_LARB0:
		return "smi_larb0";
	case DISP_MODULE_SMI_LARB4:
		return "smi_larb4";
	case DISP_MODULE_MIPI0:
		return "mipi0";
	case DISP_MODULE_MIPI1:
		return "mipi1";
	default:
		DDPMSG("invalid module id=%d", module);
		return "unknown";
	}
}

enum DISP_MODULE_ENUM ddp_get_reg_module(enum DISP_REG_ENUM reg_module)
{
	switch (reg_module) {
	case DISP_REG_CONFIG:
		return DISP_MODULE_CONFIG;
	case DISP_REG_OVL0:
		return DISP_MODULE_OVL0;
	case DISP_REG_OVL1:
		return DISP_MODULE_OVL1;
	case DISP_REG_OVL0_2L:
		return DISP_MODULE_OVL0_2L;
	case DISP_REG_OVL1_2L:
		return DISP_MODULE_OVL1_2L;
	case DISP_REG_RDMA0:
		return DISP_MODULE_RDMA0;
	case DISP_REG_RDMA1:
		return DISP_MODULE_RDMA1;
	case DISP_REG_RDMA2:
		return DISP_MODULE_RDMA2;
	case DISP_REG_WDMA0:
		return DISP_MODULE_WDMA0;
	case DISP_REG_WDMA1:
		return DISP_MODULE_WDMA1;
	case DISP_REG_COLOR0:
		return DISP_MODULE_COLOR0;
	case DISP_REG_COLOR1:
		return DISP_MODULE_COLOR1;
	case DISP_REG_CCORR0:
		return DISP_MODULE_CCORR0;
	case DISP_REG_CCORR1:
		return DISP_MODULE_CCORR1;
	case DISP_REG_AAL0:
		return DISP_MODULE_AAL0;
	case DISP_REG_AAL1:
		return DISP_MODULE_AAL1;
	case DISP_REG_GAMMA0:
		return DISP_MODULE_GAMMA0;
	case DISP_REG_GAMMA1:
		return DISP_MODULE_GAMMA1;
	case DISP_REG_OD:
		return DISP_MODULE_OD;
	case DISP_REG_DITHER0:
		return DISP_MODULE_DITHER0;
	case DISP_REG_DITHER1:
		return DISP_MODULE_DITHER1;
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
	case DISP_REG_SMI_LARB4:
		return DISP_MODULE_SMI_LARB4;
	case DISP_REG_SMI_COMMON:
		return DISP_MODULE_SMI_COMMON;
	case DISP_REG_MIPI0:
		return DISP_MODULE_MIPI0;
	case DISP_REG_MIPI1:
		return DISP_MODULE_MIPI1;
	default:
		DDPERR("%s: invalid reg module id=%d\n", __func__, reg_module);
		return DISP_MODULE_UNKNOWN;
	}
}

char *ddp_get_reg_module_name(enum DISP_REG_ENUM reg_module)
{
	return ddp_get_module_name(ddp_get_reg_module(reg_module));
}

int ddp_get_module_max_irq_bit(enum DISP_MODULE_ENUM module)
{
	switch (module) {
	case DISP_MODULE_OVL0:
		return 14;
	case DISP_MODULE_OVL1:
		return 14;
	case DISP_MODULE_OVL0_2L:
		return 14;
	case DISP_MODULE_OVL1_2L:
		return 14;
	case DISP_MODULE_RDMA0:
		return 7;
	case DISP_MODULE_RDMA1:
		return 7;
	case DISP_MODULE_RDMA2:
		return 7;
	case DISP_MODULE_WDMA0:
		return 1;
	case DISP_MODULE_WDMA1:
		return 1;
	case DISP_MODULE_COLOR0:
		return 2; /* PQ ??? */
	case DISP_MODULE_COLOR1:
		return 2; /* PQ ??? */
	case DISP_MODULE_CCORR0:
		return 0; /* PQ ??? */
	case DISP_MODULE_CCORR1:
		return 0; /* PQ ??? */
	case DISP_MODULE_AAL0:
		return 1; /* PQ ??? */
	case DISP_MODULE_AAL1:
		return 1; /* PQ ??? */
	case DISP_MODULE_GAMMA0:
		return 0; /* PQ ??? */
	case DISP_MODULE_GAMMA1:
		return 0; /* PQ ??? */
	case DISP_MODULE_OD:
		return 0; /* PQ ??? */
	case DISP_MODULE_DITHER0:
		return 0; /* PQ ??? */
	case DISP_MODULE_DITHER1:
		return 0; /* PQ ??? */
	case DISP_MODULE_DPI:
		return 2; /* DPI ??? */
	case DISP_MODULE_DSI0:
		return 12;
	case DISP_MODULE_DSI1:
		return 12;
	case DISP_MODULE_DSIDUAL:
		return 12;
	case DISP_MODULE_PWM0:
		return 0;
	case DISP_MODULE_MUTEX:
		return 14;
	default:
		DDPMSG("invalid module id=%d", module);
	}
	return 0;
}

unsigned int ddp_module_to_idx(int module)
{
	unsigned int id = 0;

	switch (module) {
	case DISP_MODULE_AAL0:
	case DISP_MODULE_COLOR0:
	case DISP_MODULE_RDMA0:
	case DISP_MODULE_WDMA0:
	case DISP_MODULE_OVL0:
	case DISP_MODULE_GAMMA0:
	case DISP_MODULE_PWM0:
	case DISP_MODULE_OD:
	case DISP_MODULE_SPLIT0:
	case DISP_MODULE_DSI0:
	case DISP_MODULE_DPI:
	case DISP_MODULE_DITHER0:
	case DISP_MODULE_CCORR0:
		id = 0;
		break;

	case DISP_MODULE_COLOR1:
	case DISP_MODULE_RDMA1:
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

/*#define DISP_NO_DPI*/ /* FIXME: tmp define */
struct DDP_MODULE_DRIVER *ddp_modules_driver[DISP_MODULE_NUM] = {
	&ddp_driver_ovl, /* DISP_MODULE_OVL0 = 0, */
	&ddp_driver_ovl, /* DISP_MODULE_OVL1  , */
	&ddp_driver_ovl, /* DISP_MODULE_OVL0_2L  , */
	&ddp_driver_ovl, /* DISP_MODULE_OVL1_2L  , */
	0,		     /* DISP_MODULE_OVL0_VIRTUAL */

	&ddp_driver_rdma, /* DISP_MODULE_RDMA0 , */
	&ddp_driver_rdma, /* DISP_MODULE_RDMA1 , */
	&ddp_driver_rdma, /* DISP_MODULE_RDMA2 , */
	&ddp_driver_wdma, /* DISP_MODULE_WDMA0 , */
	&ddp_driver_wdma, /* DISP_MODULE_WDMA1 , */

	&ddp_driver_color, /* DISP_MODULE_COLOR0, */
	NULL,	      /* DISP_MODULE_COLOR1, */
	&ddp_driver_ccorr, /* DISP_MODULE_CCORR0 , */
	NULL,	      /* DISP_MODULE_CCORR1 , */
	&ddp_driver_aal,   /* DISP_MODULE_AAL   , */

	NULL,		/* DISP_MODULE_AAL   , */
	&ddp_driver_gamma,  /* DISP_MODULE_GAMMA0 , */
	NULL,		/* DISP_MODULE_GAMMA1 , */
	&ddp_driver_od,     /* DISP_MODULE_OD , */
	&ddp_driver_dither, /* DISP_MODULE_DITHER0, */

	NULL,	     /* DISP_MODULE_DITHER1, */
	0,		      /* DISP_PATH0 */
	0,		      /* DISP_PATH1 */
	&ddp_driver_ufoe, /* DISP_MODULE_UFOE  */
	0,		      /* DISP_MODULE_DSC */

	&ddp_driver_split, /* DISP_MODULE_SPLIT0, */
#ifndef DISP_NO_DPI
	&ddp_driver_dpi, /* DISP_MODULE_DPI   , */
#else
	0,
#endif
	&ddp_driver_dsi0,    /* DISP_MODULE_DSI0  , */
	&ddp_driver_dsi1,    /* DISP_MODULE_DSI1  , */
	&ddp_driver_dsidual, /* DISP_MODULE_DSIDUAL, */

	&ddp_driver_pwm, /* DISP_MODULE_PWM0   , */
	0,		     /* DISP_MODULE_CONFIG, */
	0,		     /* DISP_MODULE_MUTEX, */
	0,		     /* DISP_MODULE_SMI_COMMON, */
	0,		     /* DISP_MODULE_SMI_LARB0 , */

	0, /* DISP_MODULE_SMI_LARB4 , */
	0, /* DISP_MODULE_MIPI0 */
	0, /* DISP_MODULE_MIPI1 */
	0, /* DISP_MODULE_UNKNOWN, */
};

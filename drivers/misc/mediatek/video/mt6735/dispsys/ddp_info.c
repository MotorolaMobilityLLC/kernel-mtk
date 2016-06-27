#define LOG_TAG "INFO"
#include "disp_drv_platform.h"
#include "ddp_info.h"
#include "ddp_debug.h"
#include "ddp_log.h"

#include "ddp_dsi.h"
#include "ddp_ovl.h"
#include "ddp_rdma.h"
#include "ddp_rdma_ex.h"
#include "ddp_wdma.h"
#include "ddp_wdma_ex.h"

char *ddp_get_module_name(DISP_MODULE_ENUM module)
{
	switch (module) {
	case DISP_MODULE_UFOE:
		return "ufoe ";
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
	case DISP_MODULE_WDMA0:
		return "wdma0 ";
	case DISP_MODULE_WDMA1:
		return "wdma1 ";
	case DISP_MODULE_OVL0:
		return "ovl0 ";
	case DISP_MODULE_OVL1:
		return "ovl1 ";
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
		DDPMSG("invalid module id=%d", module);
		return "unknown";
	}
}

char *ddp_get_reg_module_name(DISP_REG_ENUM module)
{
	switch (module) {
	case DISP_REG_OVL0:
		return "ovl0";
	case DISP_REG_OVL1:
		return "ovl1";
	case DISP_REG_RDMA0:
		return "rdma0";
	case DISP_REG_RDMA1:
		return "rdma1";
	case DISP_REG_WDMA0:
		return "wdma0";
	case DISP_REG_COLOR:
		return "color";
	case DISP_REG_CCORR:
		return "ccorr";
	case DISP_REG_AAL:
		return "aal";
	case DISP_REG_GAMMA:
		return "gamma";
	case DISP_REG_DITHER:
		return "dither";
	case DISP_REG_UFOE:
		return "ufoe";
	case DISP_REG_PWM:
		return "pwm";
	case DISP_REG_WDMA1:
		return "wdma1";
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
	case DISP_REG_CONFIG2:
		return "config_clk";
	case DISP_REG_CONFIG3:
		return "config_pll";
	case DISP_REG_IO_DRIVING1:
		return "dpi_io_drive1";
	case DISP_REG_IO_DRIVING2:
		return "dpi_io_drive2";
	case DISP_REG_IO_DRIVING3:
		return "dpi_io_drive3";
	case DISP_REG_EFUSE:
		return "efuse";
	case DISP_REG_EFUSE_PERMISSION:
		return "efuse_permission";
	case DISP_RGE_EFUSE_KEY:
		return "efuse_key";
	case DISP_REG_MIPI:
		return "mipi";
	case DISP_REG_OD:
		return "od";
	case DISP_RGE_VENCPLL:
		return "venc_pll";
	default:
		DDPMSG("invalid module id=%d", module);
		return "unknown";
	}
}

int ddp_get_module_max_irq_bit(DISP_MODULE_ENUM module)
{
	switch (module) {
	case DISP_MODULE_UFOE:
		return 0;
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
	case DISP_MODULE_WDMA1:
		return 1;
	case DISP_MODULE_OVL0:
		return 3;
	case DISP_MODULE_OVL1:
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
	case DISP_MODULE_UFOE:
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
	case DISP_MODULE_WDMA1:
	case DISP_MODULE_OVL1:
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
		DDPERR("ddp_module_to_idx, module=0x%x\n", module);
	}

	return id;
}

DDP_MODULE_DRIVER *ddp_modules_driver[DISP_MODULE_NUM] = {
	&ddp_driver_ovl,	/* DISP_MODULE_OVL0  , */
	&ddp_driver_ovl,	/* DISP_MODULE_OVL1  , */
	&ddp_driver_rdma,	/* DISP_MODULE_RDMA0 , */
	&ddp_driver_rdma,	/* DISP_MODULE_RDMA1 , */
	&ddp_driver_wdma,	/* DISP_MODULE_WDMA0 , */
	&ddp_driver_color,	/* DISP_MODULE_COLOR0, */
	&ddp_driver_ccorr,	/* DISP_MODULE_CCORR , */
	&ddp_driver_aal,	/* DISP_MODULE_AAL   , */
	&ddp_driver_gamma,	/* DISP_MODULE_GAMMA , */
	&ddp_driver_dither,	/* DISP_MODULE_DITHER, */
	0,			/* DISP_MODULE_UFOE  , //10 */
	&ddp_driver_pwm,	/* DISP_MODULE_PWM0   , */
	0,			/* DISP_MODULE_WDMA1 , */
	&ddp_driver_dsi0,	/* DISP_MODULE_DSI0  , */
#ifndef DISP_NO_DPI
	&ddp_driver_dpi,	/* DISP_MODULE_DPI   , */
#endif
	0,			/* DISP_MODULE_SMI, */
	0,			/* DISP_MODULE_CONFIG, */
	0,			/* DISP_MODULE_CMDQ, */
	0,			/* DISP_MODULE_MUTEX, */

	0,			/* DISP_MODULE_COLOR1, */
	0,			/* DISP_MODULE_RDMA2, */
	0,			/* DISP_MODULE_PWM1, */
#if defined(MTK_FB_OD_SUPPORT)
	&ddp_driver_od,		/* DISP_MODULE_OD, */
#else
	0,			/* DISP_MODULE_OD, */
#endif
	0,			/* DISP_MODULE_MERGE, */
	0,			/* DISP_MODULE_SPLIT0, */
	0,			/* DISP_MODULE_SPLIT1, */
	0,			/* DISP_MODULE_DSI1, */
	0,			/* DISP_MODULE_DSIDUAL, */
	0,			/* DISP_MODULE_SMI_LARB0 , */
	0,			/* DISP_MODULE_SMI_COMMON, */
	0,			/* DISP_MODULE_UNKNOWN, //20 */

};

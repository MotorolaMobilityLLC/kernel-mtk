#ifndef __EXTD_PLATFORM_H__
#define __EXTD_PLATFORM_H__

#include "ddp_hal.h"


/* #define MTK_LCD_HW_3D_SUPPORT */
#define ALIGN_TO(x, n)  \
	(((x) + ((n) - 1)) & ~((n) - 1))


#define MTK_EXT_DISP_OVERLAY_SUPPORT
#define MTK_AUDIO_MULTI_CHANNEL_SUPPORT

/* /#define EXTD_DBG_USE_INNER_BUF */

#define HW_OVERLAY_COUNT  4
#define EXTD_OVERLAY_CNT  4
#define HW_DPI_VSYNC_SUPPORT 1
#define MM_MHL_DVFS

#define DISP_MODULE_RDMA DISP_MODULE_RDMA1

#define FIX_EXTD_TO_OVL_PATH EXTD_OVERLAY_CNT
#endif

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

#ifndef _H_DDP_INFO
#define _H_DDP_INFO
#include <linux/types.h>
#include <linux/wait.h>
#include "ddp_hal.h"
#include "lcm_drv.h"
#include "disp_event.h"
#include "ddp_ovl.h"
#include <disp_session.h>

#define _UFMT_ID_SHIFT			0
#define _UFMT_ID_WIDTH			8
#define _UFMT_RGBSWAP_SHIFT		(_UFMT_ID_SHIFT+_UFMT_ID_WIDTH)
#define _UFMT_RGBSWAP_WIDTH		1
#define _UFMT_BYTESWAP_SHIFT		(_UFMT_RGBSWAP_SHIFT+_UFMT_RGBSWAP_WIDTH)
#define _UFMT_BYTESWAP_WIDTH		1
#define _UFMT_FORMAT_SHIFT		(_UFMT_BYTESWAP_SHIFT+_UFMT_BYTESWAP_WIDTH)
#define _UFMT_FORMAT_WIDTH		5
#define _UFMT_VDO_SHIFT			(_UFMT_FORMAT_SHIFT+_UFMT_FORMAT_WIDTH)
#define _UFMT_VDO_WIDTH			1
#define _UFMT_BLOCK_SHIT		(_UFMT_VDO_SHIFT+_UFMT_VDO_WIDTH)
#define _UFMT_BLOCK_WIDTH		1
#define _UFMT_bpp_SHIFT			(_UFMT_BLOCK_SHIT+_UFMT_BLOCK_WIDTH)
#define _UFMT_bpp_WIDTH			6
#define _UFMT_RGB_SHIFT			(_UFMT_bpp_SHIFT+_UFMT_bpp_WIDTH)
#define _UFMT_RGB_WIDTH			1


#define _MASK_SHIFT(val, width, shift) (((val)>>(shift)) & ((1<<(width))-1))

#define MAKE_UNIFIED_COLOR_FMT(rgb, bpp, block, vdo, format, byteswap, rgbswap, id) \
	( \
	((rgb)			<< _UFMT_RGB_SHIFT)	| \
	((bpp)			<< _UFMT_bpp_SHIFT)	| \
	((block)		<< _UFMT_BLOCK_SHIT)	| \
	((vdo)			<< _UFMT_VDO_SHIFT)	| \
	((format)		<< _UFMT_FORMAT_SHIFT)	| \
	((byteswap)		<< _UFMT_BYTESWAP_SHIFT)	| \
	((rgbswap)		<< _UFMT_RGBSWAP_SHIFT)	| \
	((id)			<< _UFMT_ID_SHIFT))

#define UFMT_GET_RGB(fmt)		_MASK_SHIFT(fmt, _UFMT_RGB_WIDTH, _UFMT_RGB_SHIFT)
#define UFMT_GET_bpp(fmt)		_MASK_SHIFT(fmt, _UFMT_bpp_WIDTH, _UFMT_bpp_SHIFT)
#define UFMT_GET_BLOCK(fmt)		_MASK_SHIFT(fmt, _UFMT_BLOCK_WIDTH, _UFMT_BLOCK_SHIT)
#define UFMT_GET_VDO(fmt)		_MASK_SHIFT(fmt, _UFMT_VDO_WIDTH, _UFMT_VDO_SHIFT)
#define UFMT_GET_FORMAT(fmt)		_MASK_SHIFT(fmt, _UFMT_FORMAT_WIDTH, _UFMT_FORMAT_SHIFT)
#define UFMT_GET_BYTESWAP(fmt)		_MASK_SHIFT(fmt, _UFMT_BYTESWAP_WIDTH, _UFMT_BYTESWAP_SHIFT)
#define UFMT_GET_RGBSWAP(fmt)		_MASK_SHIFT(fmt, _UFMT_RGBSWAP_WIDTH, _UFMT_RGBSWAP_SHIFT)
#define UFMT_GET_ID(fmt)		_MASK_SHIFT(fmt, _UFMT_ID_WIDTH, _UFMT_ID_SHIFT)
#define UFMT_GET_Bpp(fmt)		(UFMT_GET_bpp(fmt)/8)

unsigned int ufmt_get_rgb(unsigned int fmt);
unsigned int ufmt_get_bpp(unsigned int fmt);
unsigned int ufmt_get_block(unsigned int fmt);
unsigned int ufmt_get_vdo(unsigned int fmt);
unsigned int ufmt_get_format(unsigned int fmt);
unsigned int ufmt_get_byteswap(unsigned int fmt);
unsigned int ufmt_get_rgbswap(unsigned int fmt);
unsigned int ufmt_get_id(unsigned int fmt);
unsigned int ufmt_get_Bpp(unsigned int fmt);
unsigned int ufmt_is_old_fmt(unsigned int fmt);


enum UNIFIED_COLOR_FMT {
	UFMT_UNKNOWN = 0,
	UFMT_Y8 = MAKE_UNIFIED_COLOR_FMT(0, 8, 0, 0, 7, 0, 0, 1),
	UFMT_RGBA4444 = MAKE_UNIFIED_COLOR_FMT(1, 16, 0, 0, 0, 0, 0, 2),
	UFMT_RGBA5551 = MAKE_UNIFIED_COLOR_FMT(1, 16, 0, 0, 0, 0, 0, 3),
	UFMT_RGB565 = MAKE_UNIFIED_COLOR_FMT(1, 16, 0, 0, 0, 0, 0, 4),
	UFMT_BGR565 = MAKE_UNIFIED_COLOR_FMT(1, 16, 0, 0, 0, 1, 0, 5),
	UFMT_RGB888 = MAKE_UNIFIED_COLOR_FMT(1, 24, 0, 0, 1, 1, 0, 6),
	UFMT_BGR888 = MAKE_UNIFIED_COLOR_FMT(1, 24, 0, 0, 1, 0, 0, 7),
	UFMT_RGBA8888 = MAKE_UNIFIED_COLOR_FMT(1, 32, 0, 0, 2, 1, 0, 8),
	UFMT_BGRA8888 = MAKE_UNIFIED_COLOR_FMT(1, 32, 0, 0, 2, 0, 0, 9),
	UFMT_ARGB8888 = MAKE_UNIFIED_COLOR_FMT(1, 32, 0, 0, 3, 1, 0, 10),
	UFMT_ABGR8888 = MAKE_UNIFIED_COLOR_FMT(1, 32, 0, 0, 3, 0, 0, 11),
	UFMT_RGBX8888 = MAKE_UNIFIED_COLOR_FMT(1, 32, 0, 0, 0, 0, 0, 12),
	UFMT_BGRX8888 = MAKE_UNIFIED_COLOR_FMT(1, 32, 0, 0, 0, 0, 0, 13),
	UFMT_XRGB8888 = MAKE_UNIFIED_COLOR_FMT(1, 32, 0, 0, 0, 0, 0, 14),
	UFMT_XBGR8888 = MAKE_UNIFIED_COLOR_FMT(1, 32, 0, 0, 0, 0, 0, 15),
	UFMT_AYUV = MAKE_UNIFIED_COLOR_FMT(0, 0, 0, 0, 0, 0, 0, 16),
	UFMT_YUV = MAKE_UNIFIED_COLOR_FMT(0, 0, 0, 0, 0, 0, 0, 17),
	UFMT_UYVY = MAKE_UNIFIED_COLOR_FMT(0, 16, 0, 0, 4, 0, 0, 18),
	UFMT_VYUY = MAKE_UNIFIED_COLOR_FMT(0, 16, 0, 0, 4, 1, 0, 19),
	UFMT_YUYV = MAKE_UNIFIED_COLOR_FMT(0, 16, 0, 0, 5, 0, 0, 20),
	UFMT_YVYU = MAKE_UNIFIED_COLOR_FMT(0, 16, 0, 0, 5, 1, 0, 21),
	UFMT_UYVY_BLK = MAKE_UNIFIED_COLOR_FMT(0, 16, 1, 0, 4, 0, 0, 22),
	UFMT_VYUY_BLK = MAKE_UNIFIED_COLOR_FMT(0, 16, 1, 0, 4, 1, 0, 23),
	UFMT_YUY2_BLK = MAKE_UNIFIED_COLOR_FMT(0, 16, 1, 0, 5, 0, 0, 24),
	UFMT_YVYU_BLK = MAKE_UNIFIED_COLOR_FMT(0, 16, 1, 0, 5, 1, 0, 25),
	UFMT_YV12 = MAKE_UNIFIED_COLOR_FMT(0, 8, 0, 0, 8, 1, 0, 26),
	UFMT_I420 = MAKE_UNIFIED_COLOR_FMT(0, 8, 0, 0, 8, 0, 0, 27),
	UFMT_YV16 = MAKE_UNIFIED_COLOR_FMT(0, 8, 0, 0, 9, 1, 0, 28),
	UFMT_I422 = MAKE_UNIFIED_COLOR_FMT(0, 8, 0, 0, 9, 0, 0, 29),
	UFMT_YV24 = MAKE_UNIFIED_COLOR_FMT(0, 8, 0, 0, 10, 1, 0, 30),
	UFMT_I444 = MAKE_UNIFIED_COLOR_FMT(0, 8, 0, 0, 10, 0, 0, 31),
	UFMT_NV12 = MAKE_UNIFIED_COLOR_FMT(0, 8, 0, 0, 12, 0, 0, 32),
	UFMT_NV21 = MAKE_UNIFIED_COLOR_FMT(0, 8, 0, 0, 12, 1, 0, 33),
	UFMT_NV12_BLK = MAKE_UNIFIED_COLOR_FMT(0, 8, 1, 0, 12, 0, 0, 34),
	UFMT_NV21_BLK = MAKE_UNIFIED_COLOR_FMT(0, 8, 1, 0, 12, 1, 0, 35),
	UFMT_NV12_BLK_FLD = MAKE_UNIFIED_COLOR_FMT(0, 8, 1, 1, 12, 0, 0, 36),
	UFMT_NV21_BLK_FLD = MAKE_UNIFIED_COLOR_FMT(0, 8, 1, 1, 12, 1, 0, 37),
	UFMT_NV16 = MAKE_UNIFIED_COLOR_FMT(0, 8, 0, 0, 13, 0, 0, 38),
	UFMT_NV61 = MAKE_UNIFIED_COLOR_FMT(0, 8, 0, 0, 13, 1, 0, 39),
	UFMT_NV24 = MAKE_UNIFIED_COLOR_FMT(0, 8, 0, 0, 14, 0, 0, 40),
	UFMT_NV42 = MAKE_UNIFIED_COLOR_FMT(0, 8, 0, 0, 14, 1, 0, 41),
	UFMT_PARGB8888 = MAKE_UNIFIED_COLOR_FMT(1, 32, 0, 0, 3, 1, 0, 42),
	UFMT_PABGR8888 = MAKE_UNIFIED_COLOR_FMT(1, 32, 0, 0, 3, 1, 1, 43),
	UFMT_PRGBA8888 = MAKE_UNIFIED_COLOR_FMT(1, 32, 0, 0, 3, 0, 1, 44),
	UFMT_PBGRA8888 = MAKE_UNIFIED_COLOR_FMT(1, 32, 0, 0, 3, 0, 0, 45),
};
char *unified_color_fmt_name(enum UNIFIED_COLOR_FMT fmt);
enum UNIFIED_COLOR_FMT display_fmt_reg_to_unified_fmt(int fmt_reg_val, int byteswap,
		int rgbswap);
int is_unified_color_fmt_supported(enum UNIFIED_COLOR_FMT ufmt);
enum UNIFIED_COLOR_FMT disp_fmt_to_unified_fmt(DISP_FORMAT src_fmt);
int ufmt_disable_X_channel(enum UNIFIED_COLOR_FMT src_fmt, enum UNIFIED_COLOR_FMT *dst_fmt,
	int *const_bld);
int ufmt_disable_P(enum UNIFIED_COLOR_FMT src_fmt, enum UNIFIED_COLOR_FMT *dst_fmt);


typedef struct _OVL_CONFIG_STRUCT {
	/* unsigned int ovl_index; */
	unsigned int layer;
	unsigned int layer_en;
	enum OVL_LAYER_SOURCE source;
	enum UNIFIED_COLOR_FMT fmt;
	unsigned long addr;
	unsigned long vaddr;
	unsigned int src_x;
	unsigned int src_y;
	unsigned int src_w;
	unsigned int src_h;
	unsigned int src_pitch;
	unsigned int dst_x;
	unsigned int dst_y;
	unsigned int dst_w;
	unsigned int dst_h;
	unsigned int keyEn;
	unsigned int key;
	unsigned int aen;
	unsigned char alpha;

	unsigned int sur_aen;
	unsigned int src_alpha;
	unsigned int dst_alpha;

	unsigned int isTdshp;
	unsigned int isDirty;

	unsigned int buff_idx;
	unsigned int identity;
	unsigned int connected_type;
	DISP_BUFFER_TYPE security;
	unsigned int yuv_range;
	int is_configured;	/* is this layer configured to OVL HW, for multiply OVL sync */
	int const_bld;
} OVL_CONFIG_STRUCT;

typedef struct _OVL_BASIC_STRUCT {
	unsigned int layer;
	unsigned int layer_en;
	enum UNIFIED_COLOR_FMT fmt;
	unsigned long addr;
	unsigned int src_w;
	unsigned int src_h;
	unsigned int src_pitch;
	unsigned int bpp;
	unsigned int gpu_mode;
	unsigned int adobe_mode;
	unsigned int ovl_gamma_out;
	unsigned int alpha;
} OVL_BASIC_STRUCT;

typedef struct _RDMA_BASIC_STRUCT {
	unsigned long addr;
	unsigned int src_w;
	unsigned int src_h;
	unsigned int bpp;
} RDMA_BASIC_STRUCT;

struct rdma_bg_ctrl_t {
	unsigned int left;
	unsigned int right;
	unsigned int top;
	unsigned int bottom;
};

typedef struct _RDMA_CONFIG_STRUCT {
	unsigned idx;		/* instance index */
	enum UNIFIED_COLOR_FMT inputFormat;
	unsigned long address;
	unsigned pitch;
	unsigned width;
	unsigned height;
	unsigned dst_w;
	unsigned dst_h;
	unsigned dst_x;
	unsigned dst_y;
	DISP_BUFFER_TYPE security;
	unsigned int yuv_range;
	struct rdma_bg_ctrl_t bg_ctrl;
} RDMA_CONFIG_STRUCT;

typedef struct _WDMA_CONFIG_STRUCT {
	unsigned srcWidth;
	unsigned srcHeight;	/* input */
	unsigned clipX;
	unsigned clipY;
	unsigned clipWidth;
	unsigned clipHeight;	/* clip */
	enum UNIFIED_COLOR_FMT outputFormat;
	unsigned long dstAddress;
	unsigned dstPitch;	/* output */
	unsigned int useSpecifiedAlpha;
	unsigned char alpha;
	DISP_BUFFER_TYPE security;
} WDMA_CONFIG_STRUCT;

typedef struct {
	unsigned int fifo_mode;
	unsigned int is_wrot_sram;
	unsigned int mmsys_clk;
	unsigned int hrt_num;
	unsigned int ext_hrt_num;
	unsigned int is_display_idle;
	unsigned int is_dc;
	unsigned int hrt_magicnum; /* by resolution */
	unsigned int ext_hrt_magicnum; /* by resolution */
	unsigned int dst_width;
	unsigned int dst_height;
	unsigned int ext_dst_width;
	unsigned int ext_dst_height;
	unsigned int fps;
	unsigned int is_one_layer;
	unsigned int rdma_width;
	unsigned int rdma_height;
} golden_setting_context;

typedef struct {
	struct task_struct  *primary_display_idlemgr_task;
	wait_queue_head_t  idlemgr_wait_queue;
	unsigned long long idlemgr_last_kick_time;
	unsigned int enterulps;
	int session_mode_before_enter_idle;
	int is_primary_idle;

} disp_idlemgr_context;

typedef struct {
	/* for ovl */
	bool ovl_dirty;
	bool rdma_dirty;
	bool wdma_dirty;
	bool dst_dirty;
	int ovl_layer_dirty;	/*each bit represent one layer */
	int ovl_layer_scanned;	/*each bit reprsent one layer, used for ovl engines */
	int overlap_layer_num;
	OVL_CONFIG_STRUCT ovl_config[TOTAL_OVL_LAYER_NUM];
	RDMA_CONFIG_STRUCT rdma_config;
	WDMA_CONFIG_STRUCT wdma_config;
	LCM_PARAMS dispif_config;
	unsigned int lcm_bpp;
	unsigned int dst_w;
	unsigned int dst_h;
	unsigned int fps;
	golden_setting_context *p_golden_setting_context;
	void *path_handle;
} disp_ddp_path_config;

/* dpmgr_ioctl cmd definition */
typedef enum {
/* DSI operation */
	DDP_SWITCH_DSI_MODE = 0,
	DDP_STOP_VIDEO_MODE = 1,
	DDP_BACK_LIGHT = 2,
	DDP_SWITCH_LCM_MODE = 3,
	DDP_DPI_FACTORY_TEST = 4,
	DDP_DSI_IDLE_CLK_CLOSED = 5,
	DDP_DSI_IDLE_CLK_OPEN = 6,
	DDP_DSI_PORCH_CHANGE = 7,
	DDP_PHY_CLK_CHANGE = 8,
	DDP_ENTER_ULPS = 9,
	DDP_EXIT_ULPS = 10,
	DDP_RDMA_GOLDEN_SETTING = 11,
	DDP_OVL_GOLDEN_SETTING,
} DDP_IOCTL_NAME;

struct ddp_io_golden_setting_arg {
	enum dst_module_type dst_mod_type;
	int is_decouple_mode;
	unsigned int dst_w;
	unsigned int dst_h;
};

typedef int (*ddp_module_notify) (DISP_MODULE_ENUM, DISP_PATH_EVENT);

typedef struct {
	DISP_MODULE_ENUM module;
	int (*init)(DISP_MODULE_ENUM module, void *handle);
	int (*deinit)(DISP_MODULE_ENUM module, void *handle);
	int (*config)(DISP_MODULE_ENUM module, disp_ddp_path_config *config, void *handle);
	int (*start)(DISP_MODULE_ENUM module, void *handle);
	int (*trigger)(DISP_MODULE_ENUM module, void *handle);
	int (*stop)(DISP_MODULE_ENUM module, void *handle);
	int (*reset)(DISP_MODULE_ENUM module, void *handle);
	int (*power_on)(DISP_MODULE_ENUM module, void *handle);
	int (*power_off)(DISP_MODULE_ENUM module, void *handle);
	int (*suspend)(DISP_MODULE_ENUM module, void *handle);
	int (*resume)(DISP_MODULE_ENUM module, void *handle);
	int (*is_idle)(DISP_MODULE_ENUM module);
	int (*is_busy)(DISP_MODULE_ENUM module);
	int (*dump_info)(DISP_MODULE_ENUM module, int level);
	int (*bypass)(DISP_MODULE_ENUM module, int bypass);
	int (*build_cmdq)(DISP_MODULE_ENUM module, void *cmdq_handle, CMDQ_STATE state);
	int (*set_lcm_utils)(DISP_MODULE_ENUM module, LCM_DRIVER *lcm_drv);
	int (*set_listener)(DISP_MODULE_ENUM module, ddp_module_notify notify);
	int (*cmd)(DISP_MODULE_ENUM module, int msg, unsigned long arg, void *handle);
	int (*ioctl)(DISP_MODULE_ENUM module, void *handle, DDP_IOCTL_NAME ioctl_cmd,
		      void *params);
	int (*enable_irq)(DISP_MODULE_ENUM module, void *handle, DDP_IRQ_LEVEL irq_level);
	int (*connect)(DISP_MODULE_ENUM module, DISP_MODULE_ENUM prev, DISP_MODULE_ENUM next,
			int connect, void *handle);

} DDP_MODULE_DRIVER;

char *ddp_get_module_name(DISP_MODULE_ENUM module);
char *ddp_get_reg_module_name(DISP_REG_ENUM reg_module);
int ddp_get_module_max_irq_bit(DISP_MODULE_ENUM module);
DISP_MODULE_ENUM ddp_get_reg_module(DISP_REG_ENUM reg_module);


/* dsi */
extern DDP_MODULE_DRIVER ddp_driver_dsi0;
/* extern DDP_MODULE_DRIVER ddp_driver_dsi1; */
/* extern DDP_MODULE_DRIVER ddp_driver_dsidual; */
/* dpi */
extern DDP_MODULE_DRIVER ddp_driver_dpi;

/* ovl */
extern DDP_MODULE_DRIVER ddp_driver_ovl;
/* rdma */
extern DDP_MODULE_DRIVER ddp_driver_rdma;
/* wdma */
extern DDP_MODULE_DRIVER ddp_driver_wdma;
/* color */
extern DDP_MODULE_DRIVER ddp_driver_color;
/* aal */
extern DDP_MODULE_DRIVER ddp_driver_aal;
/* od */
/* extern  DDP_MODULE_DRIVER ddp_driver_od; */
/* gamma */
extern DDP_MODULE_DRIVER ddp_driver_gamma;
/* dither */
extern DDP_MODULE_DRIVER ddp_driver_dither;
/* ccorr */
extern DDP_MODULE_DRIVER ddp_driver_ccorr;
/* split */
/* extern DDP_MODULE_DRIVER ddp_driver_split; */

/* pwm */
extern DDP_MODULE_DRIVER ddp_driver_pwm;
/* ufoe */
/* extern DDP_MODULE_DRIVER ddp_driver_ufoe; */

extern DDP_MODULE_DRIVER *ddp_modules_driver[DISP_MODULE_NUM];

extern int display_bias_enable(void);
extern int display_bias_regulator_init(void);

#endif

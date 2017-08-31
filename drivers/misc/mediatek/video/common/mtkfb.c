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

/*#include <generated/autoconf.h>*/
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/kthread.h>
#include <linux/vmalloc.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/suspend.h>
#include <linux/of_fdt.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/dma-buf.h>
#include <linux/uaccess.h>
#include <linux/atomic.h>
#include <asm/cacheflush.h>
#include <linux/io.h>

#include <linux/compat.h>
#include <linux/dma-mapping.h>

/* #include <linux/earlysuspend.h> */
/* #include <linux/rtpm_prio.h> */
/* #include <linux/leds-mt65xx.h> */
/* #include <asm/mach-types.h> */
/* #include "mach/mt_boot.h" */
/* #include <mach/irqs.h> */

#include <mt-plat/dma.h>
#include "disp_assert_layer.h"

#include "debug.h"

#include "ddp_hal.h"
#include "ddp_log.h"
#include "disp_drv_log.h"

#include "disp_lcm.h"
#include "mtkfb.h"
#include "mtkfb_console.h"
#include "mtkfb_fence.h"
#include "mtkfb_info.h"
#include "ddp_ovl.h"
#include "disp_drv_platform.h"
#include "primary_display.h"
#include "ddp_dump.h"
#include "display_recorder.h"
#include "fbconfig_kdebug.h"
#include "ddp_manager.h"

#include "mtk_ovl.h"
#include "ion_drv.h"
#include "ddp_drv.h"

#ifdef DISP_GPIO_DTS
#include "disp_dts_gpio.h" /* set gpio via DTS */
#endif
#include "disp_helper.h"

#define ALIGN_TO(x, n)	(((x) + ((n) - 1)) & ~((n) - 1))


/* xuecheng, remove this because we use session now */
/* mtk_dispif_info_t dispif_info[MTKFB_MAX_DISPLAY_COUNT]; */

struct notifier_block pm_nb;
unsigned int EnableVSyncLog;

static u32 MTK_FB_XRES;
static u32 MTK_FB_YRES;
static u32 MTK_FB_BPP;
static u32 MTK_FB_PAGES;
static u32 fb_xres_update;
static u32 fb_yres_update;

#define MTK_FB_XRESV (ALIGN_TO(MTK_FB_XRES, MTK_FB_ALIGNMENT))
#define MTK_FB_YRESV (ALIGN_TO(MTK_FB_YRES, MTK_FB_ALIGNMENT) * MTK_FB_PAGES)	/* For page flipping */
#define MTK_FB_BYPP  ((MTK_FB_BPP + 7) >> 3)
#define MTK_FB_LINE  (ALIGN_TO(MTK_FB_XRES, MTK_FB_ALIGNMENT) * MTK_FB_BYPP)
#define MTK_FB_SIZE  (MTK_FB_LINE * ALIGN_TO(MTK_FB_YRES, MTK_FB_ALIGNMENT))

#define MTK_FB_SIZEV (MTK_FB_LINE * ALIGN_TO(MTK_FB_YRES, MTK_FB_ALIGNMENT) * MTK_FB_PAGES)

#define CHECK_RET(expr)			\
	do {				\
		int ret = (expr);	\
		ASSERT(ret == 0);	\
	} while (0)


static size_t mtkfb_log_on = true;
#define MTKFB_LOG(fmt, arg...)					\
	do {							\
		if (mtkfb_log_on)				\
			pr_debug("DISP/MTKFB " fmt, ##arg);	\
	} while (0)

/* always show this debug info while the global debug log is off */
#define MTKFB_LOG_DBG(fmt, arg...)				\
	do {							\
		if (!mtkfb_log_on)				\
			pr_debug("DISP/MTKFB " fmt, ##arg);	\
	} while (0)

#define MTKFB_FUNC()							\
	do {								\
		if (mtkfb_log_on)					\
			pr_debug("DISP/MTKFB [Func]%s\n", __func__);	\
	} while (0)

#define PRNERR(fmt, args...)  pr_debug("DISP/MTKFB " fmt, ##args)

void mtkfb_log_enable(int enable)
{
	mtkfb_log_on = enable;
	MTKFB_LOG("mtkfb log %s\n", enable ? "enabled" : "disabled");
}

/* --------------------------------------------------------------------------- */
/* local variables */
/* --------------------------------------------------------------------------- */

unsigned long fb_pa;

static const struct timeval FRAME_INTERVAL = { 0, 30000 };	/* 33ms */

atomic_t has_pending_update = ATOMIC_INIT(0);
struct fb_overlay_layer video_layerInfo;
uint32_t dbr_backup;
uint32_t dbg_backup;
uint32_t dbb_backup;
bool fblayer_dither_needed;
bool is_ipoh_bootup;
struct fb_info *mtkfb_fbi;
struct fb_overlay_layer fb_layer_context;
struct mtk_dispif_info dispif_info[MTKFB_MAX_DISPLAY_COUNT];

/**
 * This mutex is used to prevent tearing due to page flipping when adbd is
 * reading the front buffer
 */
DEFINE_SEMAPHORE(sem_flipping);
DEFINE_SEMAPHORE(sem_early_suspend);
DEFINE_SEMAPHORE(sem_overlay_buffer);

DEFINE_MUTEX(OverlaySettingMutex);
atomic_t OverlaySettingDirtyFlag = ATOMIC_INIT(0);
atomic_t OverlaySettingApplied = ATOMIC_INIT(0);
unsigned int PanDispSettingPending;
unsigned int PanDispSettingDirty;
unsigned int PanDispSettingApplied;

DECLARE_WAIT_QUEUE_HEAD(reg_update_wq);

unsigned int need_esd_check;
DECLARE_WAIT_QUEUE_HEAD(esd_check_wq);

/* extern unsigned int disp_running; */
/* extern wait_queue_head_t disp_done_wq; */

DEFINE_MUTEX(ScreenCaptureMutex);

bool is_early_suspended;
static int sem_flipping_cnt = 1;
static int sem_early_suspend_cnt = 1;
static int vsync_cnt;

/* extern BOOL is_engine_in_suspend_mode; */
/* extern BOOL is_lcm_in_suspend_mode; */

/* --------------------------------------------------------------------------- */
/* local function declarations */
/* --------------------------------------------------------------------------- */

static int init_framebuffer(struct fb_info *info);
static int mtkfb_get_overlay_layer_info(struct fb_overlay_layer_info *layerInfo);


/* --------------------------------------------------------------------------- */
/* Timer Routines */
/* --------------------------------------------------------------------------- */
unsigned int lcd_fps = 6000;
wait_queue_head_t screen_update_wq;


/*
 * ---------------------------------------------------------------------------
 *  mtkfb_set_lcm_inited() will be called in mt6516_board_init()
 * ---------------------------------------------------------------------------
 */
bool is_lcm_inited;
void mtkfb_set_lcm_inited(bool inited)
{
	is_lcm_inited = inited;
}

/*
 * ---------------------------------------------------------------------------
 * fbdev framework callbacks and the ioctl interface
 * ---------------------------------------------------------------------------
 */
/* Called each time the mtkfb device is opened */
static int mtkfb_open(struct fb_info *info, int user)
{
	/* NOT_REFERENCED(info); */
	/* NOT_REFERENCED(user); */
	/* DISPFUNC();*/
	DISPPRINT("%s\n", __func__);
	MSG_FUNC_ENTER();
	MSG_FUNC_LEAVE();
	return 0;
}

/* Called when the mtkfb device is closed. We make sure that any pending*/
/* gfx DMA operations are ended, before we return. */
static int mtkfb_release(struct fb_info *info, int user)
{

	/* NOT_REFERENCED(info); */
	/* NOT_REFERENCED(user); */
	DISPFUNC();

	MSG_FUNC_ENTER();
	MSG_FUNC_LEAVE();
	return 0;
}

/* Store a single color palette entry into a pseudo palette or the hardware
 * palette if one is available. For now we support only 16bpp and thus store
 * the entry only to the pseudo palette.
 */
static int mtkfb_setcolreg(u_int regno, u_int red, u_int green,
			   u_int blue, u_int transp, struct fb_info *info)
{
	int r = 0;
	unsigned bpp, m;

	/* NOT_REFERENCED(transp); */

	MSG_FUNC_ENTER();

	bpp = info->var.bits_per_pixel;
	m = 1 << bpp;
	if (regno >= m) {
		r = -EINVAL;
		goto exit;
	}

	switch (bpp) {
	case 16:
		/* RGB 565 */
		((u32 *) (info->pseudo_palette))[regno] =
		    ((red & 0xF800) | ((green & 0xFC00) >> 5) | ((blue & 0xF800) >> 11));
		break;
	case 32:
		/* ARGB8888 */
		((u32 *) (info->pseudo_palette))[regno] =
		    (0xff000000) |
		    ((red & 0xFF00) << 8) | ((green & 0xFF00)) | ((blue & 0xFF00) >> 8);
		break;

		/* TODO: RGB888, BGR888, ABGR8888 */

	default:
		ASSERT(0);
	}

exit:
	MSG_FUNC_LEAVE();
	return r;
}

int mtkfb_set_backlight_level(unsigned int level)
{
	MTKFB_FUNC();
	primary_display_setbacklight(level);

	return 0;
}
EXPORT_SYMBOL(mtkfb_set_backlight_level);

int mtkfb_set_backlight_mode(unsigned int mode)
{
	MTKFB_FUNC();
	if (down_interruptible(&sem_flipping)) {
		pr_debug("[FB Driver] can't get semaphore:%d\n", __LINE__);
		return -ERESTARTSYS;
	}
	sem_flipping_cnt--;
	if (down_interruptible(&sem_early_suspend)) {
		pr_debug("[FB Driver] can't get semaphore:%d\n", __LINE__);
		sem_flipping_cnt++;
		up(&sem_flipping);
		return -ERESTARTSYS;
	}

	sem_early_suspend_cnt--;
	if (primary_display_is_sleepd())
		goto End;

	/* DISP_SetBacklight_mode(mode); */
End:
	sem_flipping_cnt++;
	sem_early_suspend_cnt++;
	up(&sem_early_suspend);
	up(&sem_flipping);
	return 0;
}
EXPORT_SYMBOL(mtkfb_set_backlight_mode);


int mtkfb_set_backlight_pwm(int div)
{
	MTKFB_FUNC();
	if (down_interruptible(&sem_flipping)) {
		pr_debug("[FB Driver] can't get semaphore:%d\n", __LINE__);
		return -ERESTARTSYS;
	}
	sem_flipping_cnt--;
	if (down_interruptible(&sem_early_suspend)) {
		pr_debug("[FB Driver] can't get semaphore:%d\n", __LINE__);
		sem_flipping_cnt++;
		up(&sem_flipping);
		return -ERESTARTSYS;
	}
	sem_early_suspend_cnt--;
	if (primary_display_is_sleepd())
		goto End;
	/* DISP_SetPWM(div); */
End:
	sem_flipping_cnt++;
	sem_early_suspend_cnt++;
	up(&sem_early_suspend);
	up(&sem_flipping);
	return 0;
}
EXPORT_SYMBOL(mtkfb_set_backlight_pwm);

int mtkfb_get_backlight_pwm(int div, unsigned int *freq)
{
	/* DISP_GetPWM(div, freq); */
	return 0;
}
EXPORT_SYMBOL(mtkfb_get_backlight_pwm);

void mtkfb_waitVsync(void)
{
	if (primary_display_is_sleepd()) {
		pr_debug("[MTKFB_VSYNC]:mtkfb has suspend, return directly\n");
		msleep(20);
		return;
	}
	vsync_cnt++;
#ifdef CONFIG_FPGA_EARLY_PORTING
	msleep(20);
#else
	primary_display_wait_for_vsync(NULL);
#endif
	vsync_cnt--;
}
EXPORT_SYMBOL(mtkfb_waitVsync);

static int mtkfb_set_par(struct fb_info *fbi);

static bool no_update;

static int _convert_fb_layer_to_disp_input(struct fb_overlay_layer *src, struct disp_input_config *dst)
{
	dst->layer_id = src->layer_id;

	if (!src->layer_enable) {
		dst->layer_enable = 0;
		return 0;
	}

	switch (src->src_fmt) {
	case MTK_FB_FORMAT_YUV422:
		dst->src_fmt = DISP_FORMAT_YUV422;
		break;

	case MTK_FB_FORMAT_RGB565:
		dst->src_fmt = DISP_FORMAT_RGB565;
		break;

	case MTK_FB_FORMAT_RGB888:
		dst->src_fmt = DISP_FORMAT_RGB888;
		break;

	case MTK_FB_FORMAT_BGR888:
		dst->src_fmt = DISP_FORMAT_BGR888;
		break;

	case MTK_FB_FORMAT_ARGB8888:
		dst->src_fmt = DISP_FORMAT_ARGB8888;
		break;

	case MTK_FB_FORMAT_ABGR8888:
		dst->src_fmt = DISP_FORMAT_ABGR8888;
		break;

	case MTK_FB_FORMAT_XRGB8888:
		dst->src_fmt = DISP_FORMAT_XRGB8888;
		break;

	case MTK_FB_FORMAT_XBGR8888:
		dst->src_fmt = DISP_FORMAT_XBGR8888;
		break;

	case MTK_FB_FORMAT_UYVY:
		dst->src_fmt = DISP_FORMAT_UYVY;
		break;

	default:
		DISPERR("Invalid color format: 0x%x\n", src->src_fmt);
		return -1;
	}

	dst->src_base_addr = src->src_base_addr;
	dst->security = src->security;
	dst->src_phy_addr = src->src_phy_addr;
	DISPDBG("_convert_fb_layer_to_disp_input, dst->addr=0x%08lx\n",
		(unsigned long)(dst->src_phy_addr));

	dst->isTdshp = src->isTdshp;
	dst->next_buff_idx = src->next_buff_idx;
	dst->identity = src->identity;
	dst->connected_type = src->connected_type;

	/* set Alpha blending */
	dst->alpha = src->alpha;
	if (MTK_FB_FORMAT_ARGB8888 == src->src_fmt || MTK_FB_FORMAT_ABGR8888 == src->src_fmt)
		dst->alpha_enable = true;
	else
		dst->alpha_enable = false;

	/* set src width, src height */
	dst->src_offset_x = src->src_offset_x;
	dst->src_offset_y = src->src_offset_y;
	dst->src_width = src->src_width;
	dst->src_height = src->src_height;
	dst->tgt_offset_x = src->tgt_offset_x;
	dst->tgt_offset_y = src->tgt_offset_y;
	dst->tgt_width = src->tgt_width;
	dst->tgt_height = src->tgt_height;
	if (dst->tgt_width > dst->src_width)
		dst->tgt_width = dst->src_width;
	if (dst->tgt_height > dst->src_height)
		dst->tgt_height = dst->src_height;

	dst->src_pitch = src->src_pitch;

	/* set color key */
	dst->src_color_key = src->src_color_key;
	dst->src_use_color_key = src->src_use_color_key;

	/* data transferring is triggerred in MTKFB_TRIG_OVERLAY_OUT */
	dst->layer_enable = src->layer_enable;

#if 1
	DISPDBG("_convert_fb_layer_to_disp_input():id=%u, en=%u, next_idx=%u, vaddr=%p, paddr=%p,\n",
		dst->layer_id, dst->layer_enable, dst->next_buff_idx, dst->src_base_addr, dst->src_phy_addr);
	DISPDBG("src fmt=%u, dst fmt=%u, pitch=%u, xoff=%u, yoff=%u, w=%u, h=%u\n",
		src->src_fmt, dst->src_fmt, dst->src_pitch, dst->src_offset_x,
		dst->src_offset_y, dst->src_width, dst->src_height);
	DISPDBG("_convert_fb_layer_to_disp_input():target xoff=%u, target yoff=%u, target w=%u, target h=%u, aen=%u\n",
		dst->tgt_offset_x, dst->tgt_offset_y, dst->tgt_width, dst->tgt_height,
		dst->alpha_enable);
#endif

	return 0;
}

static int mtkfb_pan_display_impl(struct fb_var_screeninfo *var, struct fb_info *info)
{
	uint32_t offset = 0;
	uint32_t paStart = 0;
	char *vaStart = NULL, *vaEnd = NULL;
	int ret = 0;
	unsigned int src_pitch = 0;
	static unsigned int pan_display_cnt;
	struct disp_session_input_config session_input;
	struct disp_input_config *input;

	/* DISPFUNC(); */

	if (no_update) {
		DISPMSG("FB_ACTIVATE_NO_UPDATE flag found, ignore mtkfb_pan_display_impl\n");
		no_update = false;
		return ret;
	}

	DDPMLOG("pan_display: offset(%u,%u), res(%u,%u), resv(%u,%u), cnt=%d.\n",
		var->xoffset, var->yoffset, info->var.xres, info->var.yres, info->var.xres_virtual,
		info->var.yres_virtual, pan_display_cnt++);

	info->var.yoffset = var->yoffset;
	offset = var->yoffset * info->fix.line_length;
	paStart = fb_pa + offset;
	vaStart = info->screen_base + offset;
	vaEnd = vaStart + info->var.yres * info->fix.line_length;

	memset((void *)&session_input, 0, sizeof(session_input));

	/* pan display use layer 0 */
	input = &session_input.config[0];
	input->layer_id = 0;
	input->src_phy_addr = (void *)((unsigned long)paStart);
	input->src_base_addr = (void *)((unsigned long)vaStart);
	input->layer_id = primary_display_get_option("FB_LAYER");
	input->layer_enable = 1;
	input->src_offset_x = 0;
	input->src_offset_y = 0;
	input->src_width = var->xres;
	input->src_height = var->yres;
	input->tgt_offset_x = 0;
	input->tgt_offset_y = 0;
	input->tgt_width = var->xres;
	input->tgt_height = var->yres;

	switch (var->bits_per_pixel) {
	case 16:
		input->src_fmt = DISP_FORMAT_RGB565;
		break;
	case 24:
		input->src_fmt = DISP_FORMAT_RGB888;
		break;
	case 32:
		input->src_fmt = (var->blue.offset == 0) ? DISP_FORMAT_BGRA8888 : DISP_FORMAT_RGBX8888;
		break;
	default:
		DISPERR("Invalid color format bpp: %d\n", var->bits_per_pixel);
		return -1;
	}
	input->alpha_enable = false;

	input->alpha = 0xFF;
	input->next_buff_idx = -1;
	src_pitch = ALIGN_TO(var->xres, MTK_FB_ALIGNMENT);
	input->src_pitch = src_pitch;

	session_input.config_layer_num++;

	if (!is_DAL_Enabled()) {
		/* disable font layer(layer3) drawed in lk */
		session_input.config[1].layer_id = primary_display_get_option("ASSERT_LAYER");
		session_input.config[1].next_buff_idx = -1;
		session_input.config[1].layer_enable = 0;
		session_input.config_layer_num++;
	}

	ret = primary_display_config_input_multiple(&session_input);
	ret = primary_display_trigger(true, NULL, 0);
	/* primary_display_diagnose(); */

#ifdef XXXX_TODO
#error "need to wait rdma0 done here"
#error "aee dynamic switch, set overlay race condition protection"
#endif

	return ret;
}

/**
 * Set fb_info.fix fields and also updates fbdev.
 * When calling this fb_info.var must be set up already.
 */
static void set_fb_fix(struct mtkfb_device *fbdev)
{
	struct fb_info *fbi = fbdev->fb_info;
	struct fb_fix_screeninfo *fix = &fbi->fix;
	struct fb_var_screeninfo *var = &fbi->var;
	struct fb_ops *fbops = fbi->fbops;

	strncpy(fix->id, MTKFB_DRIVER, sizeof(fix->id));
	fix->type = FB_TYPE_PACKED_PIXELS;

	switch (var->bits_per_pixel) {
	case 16:
	case 24:
	case 32:
		fix->visual = FB_VISUAL_TRUECOLOR;
		break;
	case 1:
	case 2:
	case 4:
	case 8:
		fix->visual = FB_VISUAL_PSEUDOCOLOR;
		break;
	default:
		ASSERT(0);
	}

	fix->accel = FB_ACCEL_NONE;
	fix->line_length = ALIGN_TO(var->xres_virtual, MTK_FB_ALIGNMENT) * var->bits_per_pixel / 8;
	fix->smem_len = fbdev->fb_size_in_byte;
	fix->smem_start = fbdev->fb_pa_base;

	fix->xpanstep = 0;
	fix->ypanstep = 1;

	fbops->fb_fillrect = cfb_fillrect;
	fbops->fb_copyarea = cfb_copyarea;
	fbops->fb_imageblit = cfb_imageblit;
}


/**
 * Check values in var, try to adjust them in case of out of bound values if
 * possible, or return error.
 */
static int mtkfb_check_var(struct fb_var_screeninfo *var, struct fb_info *fbi)
{
	unsigned int bpp;
	unsigned long max_frame_size;
	unsigned long line_size;

	struct mtkfb_device *fbdev = (struct mtkfb_device *)fbi->par;

	/* DISPFUNC(); */

	DISPDBG("mtkfb_check_var, xres=%u, yres=%u, xres_virtual=%u, yres_virtual=%u,\n",
		var->xres, var->yres, var->xres_virtual, var->yres_virtual);
	DISPDBG("xoffset=%u, yoffset=%u, bits_per_pixel=%u\n", var->xoffset, var->yoffset, var->bits_per_pixel);

	bpp = var->bits_per_pixel;

	if (bpp != 16 && bpp != 24 && bpp != 32) {
		MTKFB_LOG("[%s]unsupported bpp: %d", __func__, bpp);
		return -1;
	}

	switch (var->rotate) {
	case 0:
	case 180:
		var->xres = MTK_FB_XRES;
		var->yres = MTK_FB_YRES;
		break;
	case 90:
	case 270:
		var->xres = MTK_FB_YRES;
		var->yres = MTK_FB_XRES;
		break;
	default:
		return -1;
	}

	if (var->xres_virtual < var->xres)
		var->xres_virtual = var->xres;
	if (var->yres_virtual < var->yres)
		var->yres_virtual = var->yres;

	max_frame_size = fbdev->fb_size_in_byte;
	DISPDBG("fbdev->fb_size_in_byte=0x%08lx\n", fbdev->fb_size_in_byte);
	line_size = var->xres_virtual * bpp / 8;

	if (line_size * var->yres_virtual > max_frame_size) {
		/* Try to keep yres_virtual first */
		line_size = max_frame_size / var->yres_virtual;
		var->xres_virtual = line_size * 8 / bpp;
		if (var->xres_virtual < var->xres) {
			/* Still doesn't fit. Shrink yres_virtual too */
			var->xres_virtual = var->xres;
			line_size = var->xres * bpp / 8;
			var->yres_virtual = max_frame_size / line_size;
		}
	}
	DISPDBG("mtkfb_check_var, xres=%u, yres=%u, xres_virtual=%u, yres_virtual=%u,\n",
		var->xres, var->yres, var->xres_virtual, var->yres_virtual);
	DISPDBG("xoffset=%u, yoffset=%u, bits_per_pixel=%u\n", var->xoffset, var->yoffset, var->bits_per_pixel);
	if (var->xres + var->xoffset > var->xres_virtual)
		var->xoffset = var->xres_virtual - var->xres;
	if (var->yres + var->yoffset > var->yres_virtual)
		var->yoffset = var->yres_virtual - var->yres;

	DISPDBG("mtkfb_check_var, xres=%u, yres=%u, xres_virtual=%u, yres_virtual=%u,\n",
		var->xres, var->yres, var->xres_virtual, var->yres_virtual);
	DISPDBG("xoffset=%u, yoffset=%u, bits_per_pixel=%u\n", var->xoffset, var->yoffset, var->bits_per_pixel);

	if (bpp == 16) {
		var->red.offset = 11;
		var->red.length = 5;
		var->green.offset = 5;
		var->green.length = 6;
		var->blue.offset = 0;
		var->blue.length = 5;
		var->transp.offset = 0;
		var->transp.length = 0;
	} else if (bpp == 24) {
		var->red.length = var->green.length = var->blue.length = 8;
		var->transp.length = 0;

		/* Check if format is RGB565 or BGR565 */

		ASSERT(var->green.offset == 8);
		ASSERT(var->red.offset + var->blue.offset == 16);
		ASSERT(var->red.offset == 16 || var->red.offset == 0);
	} else if (bpp == 32) {
		var->red.length = var->green.length = var->blue.length = var->transp.length = 8;

		/* Check if format is ARGB565 or ABGR565 */

		ASSERT(var->green.offset == 8 && var->transp.offset == 24);
		ASSERT(var->red.offset + var->blue.offset == 16);
		ASSERT(var->red.offset == 16 || var->red.offset == 0);
	}

	var->red.msb_right = 0;
	var->green.msb_right = 0;
	var->blue.msb_right = 0;
	var->transp.msb_right = 0;

	if (var->activate & FB_ACTIVATE_NO_UPDATE)
		no_update = true;
	else
		no_update = false;

	var->activate = FB_ACTIVATE_NOW;

	var->height = UINT_MAX;
	var->width = UINT_MAX;
	var->grayscale = 0;
	var->nonstd = 0;

	var->pixclock = UINT_MAX;
	var->left_margin = UINT_MAX;
	var->right_margin = UINT_MAX;
	var->upper_margin = UINT_MAX;
	var->lower_margin = UINT_MAX;
	var->hsync_len = UINT_MAX;
	var->vsync_len = UINT_MAX;

	var->vmode = FB_VMODE_NONINTERLACED;
	var->sync = 0;

	MSG_FUNC_LEAVE();
	return 0;
}

unsigned int FB_LAYER = 2;

/* Switch to a new mode. The parameters for it has been check already by
 * mtkfb_check_var.
 */
static int mtkfb_set_par(struct fb_info *fbi)
{
	struct fb_var_screeninfo *var = &fbi->var;
	struct mtkfb_device *fbdev = (struct mtkfb_device *)fbi->par;
	struct fb_overlay_layer fb_layer;
	u32 bpp = var->bits_per_pixel;
	struct disp_session_input_config session_input;
	struct disp_input_config *input;

	/* DISPFUNC(); */
	memset(&fb_layer, 0, sizeof(struct fb_overlay_layer));
	switch (bpp) {
	case 16:
		fb_layer.src_fmt = MTK_FB_FORMAT_RGB565;
		fb_layer.src_use_color_key = 1;
		fb_layer.src_color_key = 0xFF000000;
		break;

	case 24:
		fb_layer.src_use_color_key = 1;
		fb_layer.src_fmt = (var->blue.offset == 0) ?  MTK_FB_FORMAT_RGB888 : MTK_FB_FORMAT_BGR888;
		fb_layer.src_color_key = 0xFF000000;
		break;

	case 32:
		fb_layer.src_use_color_key = 0;
		DISPDBG("set_par,var->blue.offset=%d\n", var->blue.offset);
		fb_layer.src_fmt = (var->blue.offset == 0) ?  MTK_FB_FORMAT_ARGB8888 : MTK_FB_FORMAT_ABGR8888;
		fb_layer.src_color_key = 0;
		break;

	default:
		fb_layer.src_fmt = MTK_FB_FORMAT_UNKNOWN;
		DISPERR("[%s]unsupported bpp: %d", __func__, bpp);
		return -1;
	}

	set_fb_fix(fbdev);

	fb_layer.layer_id = primary_display_get_option("FB_LAYER");
	fb_layer.layer_enable = 1;
	fb_layer.src_base_addr = (void *)((unsigned long)fbdev->fb_va_base + var->yoffset * fbi->fix.line_length);
	DISPDBG("fb_pa=0x%08lx, var->yoffset=0x%08x,fbi->fix.line_length=0x%08x\n",
		fb_pa, var->yoffset, fbi->fix.line_length);
	fb_layer.src_phy_addr = (void *)(fb_pa + var->yoffset * fbi->fix.line_length);
	fb_layer.src_direct_link = 0;
	fb_layer.src_offset_x = fb_layer.src_offset_y = 0;
	/* fb_layer.src_width = fb_layer.tgt_width = fb_layer.src_pitch = var->xres; */
	/* xuecheng, does HWGPU_SUPPORT still in use now? */
#if defined(HWGPU_SUPPORT)
	fb_layer.src_pitch = ALIGN_TO(var->xres, MTK_FB_ALIGNMENT);
#else
#ifndef DISP_NO_MT_BOOT
	if (get_boot_mode() == META_BOOT || get_boot_mode() == FACTORY_BOOT ||
	    get_boot_mode() == ADVMETA_BOOT || get_boot_mode() == RECOVERY_BOOT)
		fb_layer.src_pitch = ALIGN_TO(var->xres, MTK_FB_ALIGNMENT);
	else
		fb_layer.src_pitch = ALIGN_TO(var->xres, MTK_FB_ALIGNMENT);
#endif
#endif
	fb_layer.src_width = fb_layer.tgt_width = var->xres;
	fb_layer.src_height = fb_layer.tgt_height = var->yres;
	fb_layer.tgt_offset_x = fb_layer.tgt_offset_y = 0;

	/* fb_layer.src_color_key = 0; */
	fb_layer.layer_rotation = MTK_FB_ORIENTATION_0;
	fb_layer.layer_type = LAYER_2D;
	DISPDBG("mtkfb_set_par, fb_layer.src_fmt=%x\n", fb_layer.src_fmt);

	memset((void *)&session_input, 0, sizeof(session_input));
	session_input.config_layer_num = 0;

	if (!isAEEEnabled) {
		/* DISPCHECK("AEE is not enabled, will disable layer 3\n"); */
		input = &session_input.config[session_input.config_layer_num++];
		input->layer_id = primary_display_get_option("ASSERT_LAYER");
		input->layer_enable = 0;
	} else {
		DISPCHECK("AEE is enabled, should not disable layer 3\n");
	}

	input = &session_input.config[session_input.config_layer_num++];
	_convert_fb_layer_to_disp_input(&fb_layer, input);
	primary_display_config_input_multiple(&session_input);

	/* backup fb_layer information. */
	memcpy(&fb_layer_context, &fb_layer, sizeof(fb_layer));

	MSG_FUNC_LEAVE();
	return 0;
}


static int mtkfb_soft_cursor(struct fb_info *info, struct fb_cursor *cursor)
{
	/* NOT_REFERENCED(info); */
	/* NOT_REFERENCED(cursor); */

	return 0;
}

static int mtkfb_get_overlay_layer_info(struct fb_overlay_layer_info *layerInfo)
{
#if 0
	struct DISP_LAYER_INFO layer;

	if (layerInfo->layer_id >= DDP_OVL_LAYER_MUN)
		return 0;

	layer.id = layerInfo->layer_id;
	/* DISP_GetLayerInfo(&layer); */
	int id = layerInfo->layer_id;

	layer.curr_en = captured_layer_config[id].layer_en;
	layer.next_en = cached_layer_config[id].layer_en;
	layer.hw_en = realtime_layer_config[id].layer_en;
	layer.curr_idx = captured_layer_config[id].buff_idx;
	layer.next_idx = cached_layer_config[id].buff_idx;
	layer.hw_idx = realtime_layer_config[id].buff_idx;
	layer.curr_identity = captured_layer_config[id].identity;
	layer.next_identity = cached_layer_config[id].identity;
	layer.hw_identity = realtime_layer_config[id].identity;
	layer.curr_conn_type = captured_layer_config[id].connected_type;
	layer.next_conn_type = cached_layer_config[id].connected_type;
	layer.hw_conn_type = realtime_layer_config[id].connected_type;
	layerInfo->layer_enabled = layer.hw_en;
	layerInfo->curr_en = layer.curr_en;
	layerInfo->next_en = layer.next_en;
	layerInfo->hw_en = layer.hw_en;
	layerInfo->curr_idx = layer.curr_idx;
	layerInfo->next_idx = layer.next_idx;
	layerInfo->hw_idx = layer.hw_idx;
	layerInfo->curr_identity = layer.curr_identity;
	layerInfo->next_identity = layer.next_identity;
	layerInfo->hw_identity = layer.hw_identity;
	layerInfo->curr_conn_type = layer.curr_conn_type;
	layerInfo->next_conn_type = layer.next_conn_type;
	layerInfo->hw_conn_type = layer.hw_conn_type;
#if 0
	MTKFB_LOG("[FB Driver] mtkfb_get_overlay_layer_info():id=%u, layer en=%u, next_en=%u,\n",
		  layerInfo->layer_id, layerInfo->layer_enabled, layerInfo->next_en);
	MTKFB_LOG("curr_en=%u, hw_en=%u, next_idx=%u, curr_idx=%u, hw_idx=%u\n",
		  layerInfo->curr_en, layerInfo->hw_en, layerInfo->next_idx, layerInfo->curr_idx, layerInfo->hw_idx);
#endif
#endif
	return 0;
}


#include <mt-plat/aee.h>
#define mtkfb_aee_print(string, args...)	aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_MMPROFILE_BUFFER, \
								       "sf-mtkfb blocked", string, ##args)

void mtkfb_dump_layer_info(void)
{
#if 0
	unsigned int i;

	pr_debug("[mtkfb] start dump layer info, early_suspend=%d\n", primary_display_is_sleepd());
	pr_debug("[mtkfb] cache(next):\n");
	for (i = 0; i < 4; i++) {
		pr_debug("[mtkfb] layer=%d, layer_en=%d, idx=%d, fmt=%d, addr=0x%x, %d, %d, %d\n ",
			 cached_layer_config[i].layer,	/* layer */
			 cached_layer_config[i].layer_en, cached_layer_config[i].buff_idx,
			 cached_layer_config[i].fmt,
			 cached_layer_config[i].addr,	/* addr */
			 cached_layer_config[i].identity,
			 cached_layer_config[i].connected_type, cached_layer_config[i].security);
	}

	pr_debug("[mtkfb] captured(current):\n");
	for (i = 0; i < 4; i++) {
		pr_debug("[mtkfb] layer=%d, layer_en=%d, idx=%d, fmt=%d, addr=0x%x, %d, %d, %d\n ",
			 captured_layer_config[i].layer,	/* layer */
			 captured_layer_config[i].layer_en, captured_layer_config[i].buff_idx,
			 captured_layer_config[i].fmt,
			 captured_layer_config[i].addr,	/* addr */
			 captured_layer_config[i].identity,
			 captured_layer_config[i].connected_type,
			 captured_layer_config[i].security);
	}
	pr_debug("[mtkfb] realtime(hw):\n");
	for (i = 0; i < 4; i++) {
		pr_debug("[mtkfb] layer=%d, layer_en=%d, idx=%d, fmt=%d, addr=0x%x, %d, %d, %d\n ",
			 realtime_layer_config[i].layer,	/* layer */
			 realtime_layer_config[i].layer_en, realtime_layer_config[i].buff_idx,
			 realtime_layer_config[i].fmt,
			 realtime_layer_config[i].addr,	/* addr */
			 realtime_layer_config[i].identity,
			 realtime_layer_config[i].connected_type,
			 realtime_layer_config[i].security);
	}

	/* dump mmp data */
	/* mtkfb_aee_print("surfaceflinger-mtkfb blocked"); */
#endif
}

static struct disp_session_input_config session_input;
static int mtkfb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	enum DISP_STATUS ret = 0;
	int r = 0;

	DISPFUNC();
	/* M: dump debug mmprofile log info */
	mmprofile_log_ex(MTKFB_MMP_Events.IOCtrl, MMPROFILE_FLAG_PULSE, _IOC_NR(cmd), arg);
	pr_debug("mtkfb_ioctl, info=%p, cmd nr=0x%08x, cmd size=0x%08x\n", info,
		 (unsigned int)_IOC_NR(cmd), (unsigned int)_IOC_SIZE(cmd));

	switch (cmd) {
	case MTKFB_GET_FRAMEBUFFER_MVA:
		return copy_to_user(argp, &fb_pa, sizeof(fb_pa)) ? -EFAULT : 0;
		/* remain this for engineer mode dfo multiple resolution */
#if 1
	case MTKFB_GET_DISPLAY_IF_INFORMATION:
	{
		int displayid = 0;

		if (copy_from_user(&displayid, (void __user *)arg, sizeof(displayid))) {
			MTKFB_LOG("[FB]: copy_from_user failed! line:%d\n", __LINE__);
			return -EFAULT;
		}

		if ((displayid < 0) || (displayid >= MTKFB_MAX_DISPLAY_COUNT)) {
			DISPERR("[FB]: invalid display id:%d\n", displayid);
			return -EFAULT;
		}

		if (displayid == 0) {
			dispif_info[displayid].displayWidth = primary_display_get_width();
			dispif_info[displayid].displayHeight = primary_display_get_height();

			dispif_info[displayid].lcmOriginalWidth =
			    primary_display_get_original_width();
			dispif_info[displayid].lcmOriginalHeight =
			    primary_display_get_original_height();
			dispif_info[displayid].displayMode =
			    primary_display_is_video_mode() ? 0 : 1;
		} else {
			DISPERR("information for displayid: %d is not available now\n",
				displayid);
		}

		if (copy_to_user((void __user *)arg, &(dispif_info[displayid]), sizeof(struct mtk_dispif_info))) {
			MTKFB_LOG("[FB]: copy_to_user failed! line:%d\n", __LINE__);
			r = -EFAULT;
		}

		return r;
	}
#endif
	case MTKFB_POWEROFF:
	{
		MTKFB_FUNC();
		if (primary_display_is_sleepd()) {
			pr_debug("[FB Driver] Still in MTKFB_POWEROFF!!!\n");
			return r;
		}

		pr_debug("[FB Driver] enter MTKFB_POWEROFF\n");
		/* cci400_sel_for_ddp(); */
		ret = primary_display_suspend();
		if (ret < 0)
			DISPERR("primary display suspend failed\n");

		pr_debug("[FB Driver] leave MTKFB_POWEROFF\n");

		is_early_suspended = true; /* no care */
		return ret;
	}

	case MTKFB_POWERON:
	{
		MTKFB_FUNC();
		if (primary_display_is_alive()) {
			pr_debug("[FB Driver] Still in MTKFB_POWERON!!!\n");
			return r;
		}
		pr_debug("[FB Driver] enter MTKFB_POWERON\n");
		ret = primary_display_resume();
		if (ret < 0)
			DISPERR("primary display resume failed\n");

		pr_debug("[FB Driver] leave MTKFB_POWERON\n");
		is_early_suspended = false; /* no care */
		return ret;
	}
	case MTKFB_GET_POWERSTATE:
	{
		int power_state;

		if (primary_display_is_sleepd())
			power_state = 0;
		else
			power_state = 1;

		if (copy_to_user(argp, &power_state, sizeof(power_state))) {
			pr_debug("[FB]: MTKFB_GET_POWERSTATE failed!\n");
			return -EFAULT;
		}

		return 0;
	}

	case MTKFB_CONFIG_IMMEDIATE_UPDATE:
	{
		MTKFB_LOG("[%s] MTKFB_CONFIG_IMMEDIATE_UPDATE, enable = %lu\n", __func__, arg);
		if (down_interruptible(&sem_early_suspend)) {
			MTKFB_LOG("[mtkfb_ioctl] can't get semaphore:%d\n", __LINE__);
			return -ERESTARTSYS;
		}
		sem_early_suspend_cnt--;
		/* DISP_WaitForLCDNotBusy(); */
		/* ret = DISP_ConfigImmediateUpdate((BOOL)arg); */
		/* sem_early_suspend_cnt++; */
		up(&sem_early_suspend);
		return r;
	}

#if defined(MTK_CAPTURE_SUPPORT)
	case MTKFB_CAPTURE_FRAMEBUFFER:
	{
		unsigned long dst_pbuf = 0;
		unsigned long *src_pbuf = 0;
		unsigned int pixel_bpp = info->var.bits_per_pixel / 8;
		unsigned int fbsize = DISP_GetScreenHeight() * DISP_GetScreenWidth() * pixel_bpp;

		if (copy_from_user(&dst_pbuf, (void __user *)arg, sizeof(dst_pbuf))) {
			MTKFB_LOG("[FB]: copy_from_user failed! line:%d\n", __LINE__);
			r = -EFAULT;
		} else {
			src_pbuf = vmalloc(fbsize);
			if (!src_pbuf) {
				MTKFB_LOG("[FB]: vmalloc capture src_pbuf failed! line:%d\n", __LINE__);
				r = -EFAULT;
			} else {
				dprec_logger_start(DPREC_LOGGER_WDMA_DUMP, 0, 0);
				primary_display_capture_framebuffer_ovl((unsigned long)src_pbuf, eBGRA8888);
				dprec_logger_done(DPREC_LOGGER_WDMA_DUMP, 0, 0);
				if (copy_to_user((unsigned long *)dst_pbuf, src_pbuf, fbsize)) {
					MTKFB_LOG("[FB]: copy_to_user failed! line:%d\n", __LINE__);
					r = -EFAULT;
				}
				vfree(src_pbuf);
			}
		}

		return r;
	}

	case MTKFB_SLT_AUTO_CAPTURE:
	{
		struct fb_slt_catpure capConfig;
		char *dst_buffer;
		unsigned int fb_size;

		if (copy_from_user(&capConfig, (void __user *)arg, sizeof(capConfig))) {
			MTKFB_LOG("[FB]: copy_from_user failed! line:%d\n", __LINE__);
			r = -EFAULT;
		} else {
			unsigned int format;

			switch (capConfig.format) {
			case MTK_FB_FORMAT_RGB888:
				format = eRGB888;
				break;
			case MTK_FB_FORMAT_BGR888:
				format = eBGR888;
				break;
			case MTK_FB_FORMAT_ARGB8888:
				format = eARGB8888;
				break;
			case MTK_FB_FORMAT_RGB565:
				format = eRGB565;
				break;
			case MTK_FB_FORMAT_UYVY:
				format = eYUV_420_2P_UYVY;
				break;
			case MTK_FB_FORMAT_ABGR8888:
			default:
				format = eABGR8888;
				break;
			}

			dst_buffer = (char *)capConfig.outputBuffer;
			fb_size = DISP_GetScreenWidth() * DISP_GetScreenHeight() * 4;
			if (!capConfig.outputBuffer) {
				MTKFB_LOG("[FB]: vmalloc capture outputBuffer failed! line:%d\n", __LINE__);
				r = -EFAULT;
			} else {
				capConfig.outputBuffer = vmalloc(fb_size);
				primary_display_capture_framebuffer_ovl((unsigned long)capConfig.outputBuffer, format);
				if (copy_to_user(dst_buffer, (char *)capConfig.outputBuffer, fb_size)) {
					MTKFB_LOG("[FB]: copy_to_user failed! line:%d\n", __LINE__);
					r = -EFAULT;
				}
				vfree((char *)capConfig.outputBuffer);
			}
		}

		return r;
	}
#endif

	case MTKFB_GET_OVERLAY_LAYER_INFO:
	{
		struct fb_overlay_layer_info layerInfo;

		MTKFB_LOG(" mtkfb_ioctl():MTKFB_GET_OVERLAY_LAYER_INFO\n");

		if (copy_from_user(&layerInfo, (void __user *)arg, sizeof(layerInfo))) {
			MTKFB_LOG("[FB]: copy_from_user failed! line:%d\n", __LINE__);
			return -EFAULT;
		}
		if (mtkfb_get_overlay_layer_info(&layerInfo) < 0) {
			MTKFB_LOG("[FB]: Failed to get overlay layer info\n");
			return -EFAULT;
		}
		if (copy_to_user((void __user *)arg, &layerInfo, sizeof(layerInfo))) {
			MTKFB_LOG("[FB]: copy_to_user failed! line:%d\n", __LINE__);
			r = -EFAULT;
		}
		return r;
	}
	case MTKFB_SET_OVERLAY_LAYER:
	{
		struct fb_overlay_layer layerInfo;

		if (copy_from_user(&layerInfo, (void __user *)arg, sizeof(layerInfo))) {
			MTKFB_LOG("[FB]: copy_from_user failed! line:%d\n", __LINE__);
			r = -EFAULT;
		} else {
			struct disp_input_config *input;

			memset((void *)&session_input, 0, sizeof(session_input));
			if (layerInfo.layer_id >= TOTAL_OVL_LAYER_NUM) {
				MTKFB_LOG
					("MTKFB_SET_OVERLAY_LAYER, layer_id invalid=%d\n",
					 layerInfo.layer_id);
			} else {
				input = &session_input.config[session_input.config_layer_num++];
				_convert_fb_layer_to_disp_input(&layerInfo, input);
			}


			primary_display_config_input_multiple(&session_input);
			primary_display_trigger(1, NULL, 0);
		}

		return r;
	}

	case MTKFB_ERROR_INDEX_UPDATE_TIMEOUT:
	{
		pr_debug("[DDP] mtkfb_ioctl():MTKFB_ERROR_INDEX_UPDATE_TIMEOUT\n");
		/* call info dump function here */
		/* mtkfb_dump_layer_info(); */
		return r;
	}

	case MTKFB_ERROR_INDEX_UPDATE_TIMEOUT_AEE:
	{
		pr_debug("[DDP] mtkfb_ioctl():MTKFB_ERROR_INDEX_UPDATE_TIMEOUT\n");
		/* call info dump function here */
		/* mtkfb_dump_layer_info(); */
		/* mtkfb_aee_print("surfaceflinger-mtkfb blocked"); */
		return r;
	}

	case MTKFB_SET_VIDEO_LAYERS:
	{
		struct mmp_fb_overlay_layers {
			struct fb_overlay_layer Layer0;
			struct fb_overlay_layer Layer1;
			struct fb_overlay_layer Layer2;
			struct fb_overlay_layer Layer3;
		};

		struct fb_overlay_layer layerInfo[VIDEO_LAYER_COUNT];

		MTKFB_LOG(" mtkfb_ioctl():MTKFB_SET_VIDEO_LAYERS\n");
		mmprofile_log(MTKFB_MMP_Events.SetOverlayLayers, MMPROFILE_FLAG_START);

		if (copy_from_user(&layerInfo, (void __user *)arg, sizeof(layerInfo))) {
			MTKFB_LOG("[FB]: copy_from_user failed! line:%d\n", __LINE__);
			mmprofile_log_meta_string(MTKFB_MMP_Events.SetOverlayLayers,
					       MMPROFILE_FLAG_END, "Copy_from_user failed!");
			r = -EFAULT;
		} else {
			int32_t i;
			/* mutex_lock(&OverlaySettingMutex); */
			struct disp_input_config *input;

			memset((void *)&session_input, 0, sizeof(session_input));

			for (i = 0; i < VIDEO_LAYER_COUNT; ++i) {
				if (layerInfo[i].layer_id >= OVL_LAYER_NUM) {
					DDPAEE
					    ("MTKFB_SET_VIDEO_LAYERS, layer_id invalid=%d\n",
					     layerInfo[i].layer_id);
					continue;
				}

				input =
				    &session_input.config[session_input.config_layer_num++];
				_convert_fb_layer_to_disp_input(&layerInfo[i], input);
			}
			/* is_ipoh_bootup = false; */
			/* atomic_set(&OverlaySettingDirtyFlag, 1); */
			/* atomic_set(&OverlaySettingApplied, 0); */
			/* mutex_unlock(&OverlaySettingMutex); */
			/* mmprofile_logStructure(MTKFB_MMP_Events.SetOverlayLayers, MMPROFILE_FLAG_END,*/
						/* layerInfo, struct mmp_fb_overlay_layers); */
			primary_display_config_input_multiple(&session_input);
			primary_display_trigger(1, NULL, 0);
		}

		return r;
	}

	case MTKFB_TRIG_OVERLAY_OUT:
	{
		MTKFB_LOG(" mtkfb_ioctl():MTKFB_TRIG_OVERLAY_OUT\n");
		mmprofile_log(MTKFB_MMP_Events.TrigOverlayOut, MMPROFILE_FLAG_PULSE);
		primary_display_trigger(1, NULL, 0);
		return 0;
	}

	case MTKFB_META_RESTORE_SCREEN:
	{
		struct fb_var_screeninfo var;

		if (copy_from_user(&var, argp, sizeof(var)))
			return -EFAULT;
		if (info->var.yres + var.yoffset > info->var.yres_virtual)
			return -EFAULT;

		info->var.yoffset = var.yoffset;
		init_framebuffer(info);

		return mtkfb_pan_display_impl(&var, info);
	}


	case MTKFB_GET_DEFAULT_UPDATESPEED:
	{
		unsigned int speed;

		MTKFB_LOG("[MTKFB] get default update speed\n");
		/* DISP_Get_Default_UpdateSpeed(&speed); */

		pr_debug("[MTKFB EM]MTKFB_GET_DEFAULT_UPDATESPEED is %d\n", speed);
		return copy_to_user(argp, &speed, sizeof(speed)) ? -EFAULT : 0;
	}

	case MTKFB_GET_CURR_UPDATESPEED:
	{
		unsigned int speed;

		MTKFB_LOG("[MTKFB] get current update speed\n");
		/* DISP_Get_Current_UpdateSpeed(&speed); */

		pr_debug("[MTKFB EM]MTKFB_GET_CURR_UPDATESPEED is %d\n", speed);
		return copy_to_user(argp, &speed, sizeof(speed)) ? -EFAULT : 0;
	}

	case MTKFB_CHANGE_UPDATESPEED:
	{
		unsigned int speed;

		MTKFB_LOG("[MTKFB] change update speed\n");

		if (copy_from_user(&speed, (void __user *)arg, sizeof(speed))) {
			MTKFB_LOG("[FB]: copy_from_user failed! line:%d\n", __LINE__);
			r = -EFAULT;
		} else {
			/* DISP_Change_Update(speed); */

			pr_debug("[MTKFB EM]MTKFB_CHANGE_UPDATESPEED is %d\n", speed);
		}
		return r;
	}

	case MTKFB_AEE_LAYER_EXIST:
	{
		/* pr_debug("[MTKFB] isAEEEnabled=%d\n", isAEEEnabled); */
		return copy_to_user(argp, &isAEEEnabled,
				    sizeof(isAEEEnabled)) ? -EFAULT : 0;
	}
	case MTKFB_LOCK_FRONT_BUFFER:
		return 0;
	case MTKFB_UNLOCK_FRONT_BUFFER:
		return 0;

	case MTKFB_FACTORY_AUTO_TEST:
	{
		unsigned int result = 0;

		pr_debug("factory mode: lcm auto test\n");
		result = mtkfb_fm_auto_test();
		return copy_to_user(argp, &result, sizeof(result)) ? -EFAULT : 0;
	}
	default:
		pr_debug("mtkfb_ioctl Not support, info=%p, cmd=0x%08x, arg=0x%lx\n", info,
			 (unsigned int)cmd, arg);
		return -EINVAL;
	}
}

#ifdef CONFIG_COMPAT
struct compat_fb_overlay_layer {
	compat_uint_t layer_id;
	compat_uint_t layer_enable;

	compat_uptr_t src_base_addr;
	compat_uptr_t src_phy_addr;
	compat_uint_t src_direct_link;
	compat_int_t src_fmt;
	compat_uint_t src_use_color_key;
	compat_uint_t src_color_key;
	compat_uint_t src_pitch;
	compat_uint_t src_offset_x, src_offset_y;
	compat_uint_t src_width, src_height;

	compat_uint_t tgt_offset_x, tgt_offset_y;
	compat_uint_t tgt_width, tgt_height;
	compat_int_t layer_rotation;
	compat_int_t layer_type;
	compat_int_t video_rotation;

	compat_uint_t isTdshp;	/* set to 1, will go through tdshp first, then layer blending, then to color */

	compat_int_t next_buff_idx;
	compat_int_t identity;
	compat_int_t connected_type;
	compat_uint_t security;
	compat_uint_t alpha_enable;
	compat_uint_t alpha;
	compat_int_t fence_fd;	/* 8135 */
	compat_int_t ion_fd;	/* 8135 CL 2340210 */
};

#define COMPAT_MTKFB_SET_OVERLAY_LAYER		MTK_IOW(0, struct compat_fb_overlay_layer)
#define COMPAT_MTKFB_TRIG_OVERLAY_OUT		MTK_IO(1)
#define COMPAT_MTKFB_SET_VIDEO_LAYERS		MTK_IOW(2, struct compat_fb_overlay_layer)

#define COMPAT_MTKFB_CAPTURE_FRAMEBUFFER	MTK_IOW(3, compat_ulong_t)
#define COMPAT_MTKFB_CONFIG_IMMEDIATE_UPDATE	MTK_IOW(4, compat_ulong_t)

#define COMPAT_MTKFB_GET_POWERSTATE		MTK_IOR(21, compat_ulong_t)
#define COMPAT_MTKFB_META_RESTORE_SCREEN	MTK_IOW(101, compat_ulong_t)

static void compat_convert(struct compat_fb_overlay_layer *compat_info,
			   struct fb_overlay_layer *info)
{
	info->layer_id = compat_info->layer_id;
	info->layer_enable = compat_info->layer_enable;
	info->src_base_addr = (void *)((unsigned long)compat_info->src_base_addr);
	info->src_phy_addr = (void *)((unsigned long)compat_info->src_phy_addr);
	info->src_direct_link = compat_info->src_direct_link;
	info->src_fmt = compat_info->src_fmt;
	info->src_use_color_key = compat_info->src_use_color_key;
	info->src_color_key = compat_info->src_color_key;
	info->src_pitch = compat_info->src_pitch;
	info->src_offset_x = compat_info->src_offset_x;
	info->src_offset_y = compat_info->src_offset_y;
	info->src_width = compat_info->src_width;
	info->src_height = compat_info->src_height;
	info->tgt_offset_x = compat_info->tgt_offset_x;
	info->tgt_offset_y = compat_info->tgt_offset_y;
	info->tgt_width = compat_info->tgt_width;
	info->tgt_height = compat_info->tgt_height;
	info->layer_rotation = compat_info->layer_rotation;
	info->layer_type = compat_info->layer_type;
	info->video_rotation = compat_info->video_rotation;

	info->isTdshp = compat_info->isTdshp;
	info->next_buff_idx = compat_info->next_buff_idx;
	info->identity = compat_info->identity;
	info->connected_type = compat_info->connected_type;

	info->security = compat_info->security;
	info->alpha_enable = compat_info->alpha_enable;
	info->alpha = compat_info->alpha;
	info->fence_fd = compat_info->fence_fd;
	info->ion_fd = compat_info->ion_fd;
}

static int mtkfb_compat_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	struct fb_overlay_layer layerInfo;
	long ret = 0;

	pr_debug("[FB Driver] mtkfb_compat_ioctl, cmd=0x%08x, cmd nr=0x%08x, cmd size=0x%08x\n",
		 cmd, (unsigned int)_IOC_NR(cmd), (unsigned int)_IOC_SIZE(cmd));

	switch (cmd) {
	case COMPAT_MTKFB_GET_POWERSTATE:
	{
		compat_uint_t __user *data32;
		int power_state = 0;

		data32 = compat_ptr(arg);
		if (primary_display_is_sleepd())
			power_state = 0;
		else
			power_state = 1;
		if (put_user(power_state, data32)) {
			pr_debug("MTKFB_GET_POWERSTATE failed\n");
			ret = -EFAULT;
		}
		pr_debug("MTKFB_GET_POWERSTATE success %d\n", power_state);
		break;
	}

#if defined(MTK_CAPTURE_SUPPORT)
	case COMPAT_MTKFB_CAPTURE_FRAMEBUFFER:
	{
		compat_ulong_t __user *data32;
		unsigned long *pbuf;
		unsigned int pixel_bpp = info->var.bits_per_pixel / 8;
		unsigned int fbsize = DISP_GetScreenHeight() * DISP_GetScreenWidth() * pixel_bpp;
		unsigned long dest;

		data32 = compat_ptr(arg);
		pbuf = compat_alloc_user_space(fbsize);

		if (!pbuf) {
			pr_warn("[FB]: vmalloc capture src_pbuf failed! line:%d\n", __LINE__);
			ret  = -EFAULT;
		} else {
			dprec_logger_start(DPREC_LOGGER_WDMA_DUMP, 0, 0);
			primary_display_capture_framebuffer_ovl((unsigned long)pbuf, eBGRA8888);
			dprec_logger_done(DPREC_LOGGER_WDMA_DUMP, 0, 0);
			ret = get_user(dest, data32);
			if (copy_in_user((unsigned long *)dest, pbuf, fbsize/2)) {
				pr_warn("[FB]: copy_to_user failed! line:%d\n", __LINE__);
				ret  = -EFAULT;
			}
		}
		break;
	}
#endif

	case COMPAT_MTKFB_TRIG_OVERLAY_OUT:
	{
		arg = (unsigned long)compat_ptr(arg);
		ret = mtkfb_ioctl(info, MTKFB_TRIG_OVERLAY_OUT, arg);
		break;
	}
	case COMPAT_MTKFB_META_RESTORE_SCREEN:
	{
		arg = (unsigned long)compat_ptr(arg);
		ret = mtkfb_ioctl(info, MTKFB_META_RESTORE_SCREEN, arg);
		break;
	}
	case COMPAT_MTKFB_SET_OVERLAY_LAYER:
	{
		struct compat_fb_overlay_layer compat_layerInfo;

		MTKFB_LOG(" mtkfb_compat_ioctl():MTKFB_SET_OVERLAY_LAYER\n");

		arg = (unsigned long)compat_ptr(arg);
		if (copy_from_user(&compat_layerInfo, (void __user *)arg, sizeof(compat_layerInfo))) {
			MTKFB_LOG("[FB Driver]: copy_from_user failed! line:%d\n",
				  __LINE__);
			ret = -EFAULT;
		} else {
			struct disp_input_config *input;

			compat_convert(&compat_layerInfo, &layerInfo);

			/* in early suspend mode ,will not update buffer index, info SF by return value */
			if (primary_display_is_sleepd()) {
				pr_debug("[FB Driver] error, set overlay in early suspend ,skip!\n");
				return MTKFB_ERROR_IS_EARLY_SUSPEND;
			}
			memset((void *)&session_input, 0, sizeof(session_input));
			if (layerInfo.layer_id >= TOTAL_OVL_LAYER_NUM) {
				DDPAEE
					("COMPAT_MTKFB_SET_OVERLAY_LAYER, layer_id invalid=%d\n",
					 layerInfo.layer_id);
			} else {
				input = &session_input.config[session_input.config_layer_num++];
				_convert_fb_layer_to_disp_input(&layerInfo, input);
			}
			primary_display_config_input_multiple(&session_input);
			/* primary_display_trigger(1, NULL, 0); */
		}
	}
		break;

	case COMPAT_MTKFB_SET_VIDEO_LAYERS:
	{
		struct compat_fb_overlay_layer compat_layerInfo[VIDEO_LAYER_COUNT];

		MTKFB_LOG(" mtkfb_compat_ioctl():MTKFB_SET_VIDEO_LAYERS\n");

		if (copy_from_user(&compat_layerInfo, (void __user *)arg, sizeof(compat_layerInfo))) {
			MTKFB_LOG("[FB Driver]: copy_from_user failed! line:%d\n",
				  __LINE__);
			ret = -EFAULT;
		} else {
			int32_t i;
			/* mutex_lock(&OverlaySettingMutex); */
			struct disp_input_config *input;

			memset((void *)&session_input, 0, sizeof(session_input));

			for (i = 0; i < VIDEO_LAYER_COUNT; ++i) {
				compat_convert(&compat_layerInfo[i], &layerInfo);
				if (layerInfo.layer_id >= OVL_LAYER_NUM) {
					DDPAEE
					    ("COMPAT_MTKFB_SET_VIDEO_LAYERS, layer_id invalid=%d\n",
					     layerInfo.layer_id);
					continue;
				}
				input =
				    &session_input.config[session_input.config_layer_num++];
				_convert_fb_layer_to_disp_input(&layerInfo, input);
			}
			/* is_ipoh_bootup = false; */
			/* atomic_set(&OverlaySettingDirtyFlag, 1); */
			/* atomic_set(&OverlaySettingApplied, 0); */
			/* mutex_unlock(&OverlaySettingMutex); */
			/* mmprofile_logStructure(MTKFB_MMP_Events.SetOverlayLayers, MMPROFILE_FLAG_END,*/
						 /*layerInfo, struct mmp_fb_overlay_layers); */
			primary_display_config_input_multiple(&session_input);
			/* primary_display_trigger(1, NULL, 0); */
		}
	}
		break;
	default:
		/* NOTHING DIFFERENCE with standard ioctl calling */
		arg = (unsigned long)compat_ptr(arg);
		ret = mtkfb_ioctl(info, cmd, arg);
		break;
	}

	return ret;
}
#endif

static int mtkfb_pan_display_proxy(struct fb_var_screeninfo *var, struct fb_info *info)
{
#ifdef CONFIG_MTPROF_APPLAUNCH	/* eng enable, user disable */
	pr_debug("AppLaunch mtkfb_pan_display_proxy.\n");
#endif
	return mtkfb_pan_display_impl(var, info);
}

static int mtkfb_blank_suspend(void);
static int mtkfb_blank_resume(void);

#if defined(CONFIG_PM_AUTOSLEEP)
static int mtkfb_blank(int blank_mode, struct fb_info *info)
{
	int ret = 0;

	switch (blank_mode) {
	case FB_BLANK_UNBLANK:
	case FB_BLANK_NORMAL:
		ret = mtkfb_blank_resume();
		if (!lcd_fps)
			msleep(30);
		else
			msleep(2 * 100000 / lcd_fps);	/* Delay 2 frames. */
		break;
	case FB_BLANK_VSYNC_SUSPEND:
	case FB_BLANK_HSYNC_SUSPEND:
		break;
	case FB_BLANK_POWERDOWN:
		ret = mtkfb_blank_suspend();
		break;
	default:
		return -EINVAL;
	}

	return ret;
}
#endif

/* Callback table for the frame buffer framework. Some of these pointers
 * will be changed according to the current setting of fb_info->accel_flags.
 */
static struct fb_ops mtkfb_ops = {
	.owner = THIS_MODULE,
	.fb_open = mtkfb_open,
	.fb_release = mtkfb_release,
	.fb_setcolreg = mtkfb_setcolreg,
	.fb_pan_display = mtkfb_pan_display_proxy,
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
	.fb_cursor = mtkfb_soft_cursor,
	.fb_check_var = mtkfb_check_var,
	.fb_set_par = mtkfb_set_par,
	.fb_ioctl = mtkfb_ioctl,
#ifdef CONFIG_COMPAT
	.fb_compat_ioctl = mtkfb_compat_ioctl,
#endif
#if defined(CONFIG_PM_AUTOSLEEP)
	.fb_blank = mtkfb_blank,
#endif
};

/**
 * ---------------------------------------------------------------------------
 * Sysfs interface
 * ---------------------------------------------------------------------------
 */
static int mtkfb_register_sysfs(struct mtkfb_device *fbdev)
{
	/* NOT_REFERENCED(fbdev); */

	return 0;
}

static void mtkfb_unregister_sysfs(struct mtkfb_device *fbdev)
{
	/* NOT_REFERENCED(fbdev); */
}

/**
 * ---------------------------------------------------------------------------
 * LDM callbacks
 * ---------------------------------------------------------------------------
 */
/* Initialize system fb_info object and set the default video mode.
 * The frame buffer memory already allocated by lcddma_init
 */
static int mtkfb_fbinfo_init(struct fb_info *info)
{
	struct mtkfb_device *fbdev = (struct mtkfb_device *)info->par;
	struct fb_var_screeninfo var;
	int r = 0;

	/*DISPFUNC(); */

	if (!fbdev->fb_va_base) {
		DISPERR("init fb info fail\n");
		return -1;
	}
	info->fbops = &mtkfb_ops;
	info->flags = FBINFO_FLAG_DEFAULT;
	info->screen_base = (char *)fbdev->fb_va_base;
	info->screen_size = fbdev->fb_size_in_byte;
	info->pseudo_palette = fbdev->pseudo_palette;

	r = fb_alloc_cmap(&info->cmap, 32, 0);
	if (r != 0)
		DISPERR("unable to allocate color map memory\n");

	/* setup the initial video mode (RGB565) */

	memset(&var, 0, sizeof(var));

	var.xres = MTK_FB_XRES;
	var.yres = MTK_FB_YRES;
	var.xres_virtual = MTK_FB_XRESV;
	var.yres_virtual = MTK_FB_YRESV;
	DISPMSG("FB_XRES=%d, FB_YRES=%d, FB_XRES_V=%d, FB_YRES_V=%d, BPP=%d, FB_PAGES=%d, FB_LINE=%d, FB_SIZEV=%d\n",
		MTK_FB_XRES, MTK_FB_YRES, MTK_FB_XRESV, MTK_FB_YRESV, MTK_FB_BPP, MTK_FB_PAGES, MTK_FB_LINE,
		MTK_FB_SIZEV);
	/* use 32 bit framebuffer as default */
	var.bits_per_pixel = 32;

	var.transp.offset = 24;
	var.red.length = 8;
#if 0
	var.red.offset = 16;
	var.red.length = 8;
	var.green.offset = 8;
	var.green.length = 8;
	var.blue.offset = 0;
	var.blue.length = 8;
#else
	var.red.offset = 0;
	var.red.length = 8;
	var.green.offset = 8;
	var.green.length = 8;
	var.blue.offset = 16;
	var.blue.length = 8;
#endif
	var.width = DISP_GetActiveWidth();
	var.height = DISP_GetActiveHeight();
	var.activate = FB_ACTIVATE_NOW;

	r = mtkfb_check_var(&var, info);
	if (r != 0)
		DISPERR("failed to mtkfb_check_var\n");

	info->var = var;

	r = mtkfb_set_par(info);
	if (r != 0)
		DISPERR("failed to mtkfb_set_par\n");

	MSG_FUNC_LEAVE();
	return r;
}

/* Release the fb_info object */
static void mtkfb_fbinfo_cleanup(struct mtkfb_device *fbdev)
{
	MSG_FUNC_ENTER();

	fb_dealloc_cmap(&fbdev->fb_info->cmap);

	MSG_FUNC_LEAVE();
}

#define RGB565_TO_ARGB8888(x)		\
	((((x) &   0x1F) << 3) |	\
	(((x) &  0x7E0) << 5) |		\
	(((x) & 0xF800) << 8) |		\
	(0xFF << 24)) /* opaque */

/* Init frame buffer content as 3 R/G/B color bars for debug */
static int init_framebuffer(struct fb_info *info)
{
	void *buffer = info->screen_base + info->var.yoffset * info->fix.line_length;

	/* clean whole frame buffer as black */
	int size = info->var.xres_virtual * info->var.yres * info->var.bits_per_pixel/8;

	/*memset_io(buffer, 0, info->screen_size)*/;

	memset_io(buffer, 0, size);

	return 0;
}


/**
 * Free driver resources. Can be called to rollback an aborted initialization
 * sequence.
 */
static void mtkfb_free_resources(struct mtkfb_device *fbdev, int state)
{
	int r = 0;

	switch (state) {
	case MTKFB_ACTIVE:
		r = unregister_framebuffer(fbdev->fb_info);
		ASSERT(r == 0);
		/* lint -fallthrough */
	case 5:
		mtkfb_unregister_sysfs(fbdev);
		/* lint -fallthrough */
	case 4:
		mtkfb_fbinfo_cleanup(fbdev);
		/* lint -fallthrough */
	case 3:
		/* DISP_CHECK_RET(DISP_Deinit()); */
		/* lint -fallthrough */
	case 2:
#ifndef CONFIG_FPGA_EARLY_PORTING
		dma_free_coherent(0, fbdev->fb_size_in_byte, fbdev->fb_va_base, fbdev->fb_pa_base);
#endif
		/* lint -fallthrough */
	case 1:
		dev_set_drvdata(fbdev->dev, NULL);
		framebuffer_release(fbdev->fb_info);
		/* lint -fallthrough */
	case 0:
		/* nothing to free */
		break;
	default:
		DISPERR("free resources fail, state=%d\n", state);
		ASSERT(0);
	}
}

char mtkfb_lcm_name[256] = { 0 };

void disp_get_fb_address(unsigned long *fbVirAddr, unsigned long *fbPhysAddr)
{
	struct mtkfb_device *fbdev = (struct mtkfb_device *)mtkfb_fbi->par;

	*fbVirAddr = (unsigned long)fbdev->fb_va_base + mtkfb_fbi->var.yoffset * mtkfb_fbi->fix.line_length;
	*fbPhysAddr = (unsigned long)fbdev->fb_pa_base + mtkfb_fbi->var.yoffset * mtkfb_fbi->fix.line_length;
}

char *mtkfb_find_lcm_driver(void)
{

#ifdef CONFIG_OF
	if (_parse_tag_videolfb() == 1) {
		pr_debug("[mtkfb] not found LCM driver, return NULL\n");
		return NULL;
	}
#else
	{
		char *p, *q;

		p = strstr(saved_command_line, "lcm=");
		/* we can't find lcm string in the command line, the uboot should be old version */
		if (p == NULL)
			return NULL;

		p += 6;
		if ((p - saved_command_line) > strlen(saved_command_line + 1))
			return NULL;

		pr_debug("%s, %s\n", __func__, p);
		q = p;
		while (*q != ' ' && *q != '\0')
			q++;

		memset((void *)mtkfb_lcm_name, 0, sizeof(mtkfb_lcm_name));
		strncpy((char *)mtkfb_lcm_name, (const char *)p, (int)(q - p));
		mtkfb_lcm_name[q - p + 1] = '\0';
	}
#endif
	/* printk("%s, %s\n", __func__, mtkfb_lcm_name); */
	return mtkfb_lcm_name;
}

uint32_t color;
unsigned int mtkfb_fm_auto_test(void)
{
	unsigned int result = 0;
	unsigned int i = 0;
	unsigned long fbVirAddr;
	uint32_t fbsize;
	int r = 0;
	unsigned int *fb_buffer;
	struct mtkfb_device *fbdev = (struct mtkfb_device *)mtkfb_fbi->par;
	struct fb_var_screeninfo var;

	fbVirAddr = (unsigned long)fbdev->fb_va_base;
	fb_buffer = (unsigned int *)fbVirAddr;

	memcpy(&var, &(mtkfb_fbi->var), sizeof(var));
	var.activate = FB_ACTIVATE_NOW;
	var.bits_per_pixel = 32;
	var.transp.offset = 24;
	var.transp.length = 8;
	var.red.offset = 16;
	var.red.length = 8;
	var.green.offset = 8;
	var.green.length = 8;
	var.blue.offset = 0;
	var.blue.length = 8;

	r = mtkfb_check_var(&var, mtkfb_fbi);
	if (r != 0)
		PRNERR("failed to mtkfb_check_var\n");

	mtkfb_fbi->var = var;

#if 0
	r = mtkfb_set_par(mtkfb_fbi);

	if (r != 0)
		PRNERR("failed to mtkfb_set_par\n");
#endif
	if (color == 0)
		color = 0xFF00FF00;
	fbsize = ALIGN_TO(DISP_GetScreenWidth(), MTK_FB_ALIGNMENT) * DISP_GetScreenHeight() * MTK_FB_PAGES;
	for (i = 0; i < fbsize; i++)
		*fb_buffer++ = color;
#if 0
	if (!primary_display_is_video_mode())
		primary_display_trigger(1, NULL, 0);
#endif
	mtkfb_pan_display_impl(&mtkfb_fbi->var, mtkfb_fbi);
	msleep(100);

	result = primary_display_lcm_ATA();

	if (result == 0)
		pr_debug("ATA LCM failed\n");
	else
		pr_debug("ATA LCM passed\n");

	return result;
}

#if 0
static void _mtkfb_draw_block(unsigned long addr, unsigned int x, unsigned int y, unsigned int w,
			      unsigned int h, unsigned int color)
{
	int i = 0;
	int j = 0;
	unsigned long start_addr = addr + MTK_FB_XRESV * 4 * y + x * 4;

	for (j = 0; j < h; j++) {
		for (i = 0; i < w; i++)
			/* *(unsigned long*)(start_addr + i*4 + j*MTK_FB_XRESV*4) = color; */
			mt_reg_sync_writel(color, (start_addr + i * 4 + j * MTK_FB_XRESV * 4));

	}
}

static long int get_current_time_us(void)
{
	struct timeval t;

	do_gettimeofday(&t);
	return (t.tv_sec & 0xFFF) * 1000000 + t.tv_usec;
}

static int _mtkfb_internal_test(unsigned long va, unsigned int w, unsigned int h)
{
	/* this is for debug, used in bring up day */
	unsigned int i = 0;
	unsigned int color = 0;
	int _internal_test_block_size = 120;

	for (i = 0; i < w * h / _internal_test_block_size / _internal_test_block_size; i++) {
		color = (i & 0x1) * 0xff;
		/* color += ((i&0x2)>>1)*0xff00; */
		/* color += ((i&0x4)>>2)*0xff0000; */
		color += 0xff000000U;
		_mtkfb_draw_block(va, i % (w / _internal_test_block_size) * _internal_test_block_size,
				  i / (w / _internal_test_block_size) * _internal_test_block_size,
				  _internal_test_block_size, _internal_test_block_size, color);
	}
	/* unsigned long ttt = get_current_time_us(); */
	/* for (i = 0; i < 1000; i++) */
	primary_display_trigger(1, NULL, 0);
	/* ttt = get_current_time_us() - ttt; */
	/* DISPMSG("%s, update 1000 times, fps=%2d.%2d\n",*/
		  /* __func__, (1000*100/(ttt/1000/1000))/100, (1000*100/(ttt/1000/1000))%100);*/

	return 0;

	_internal_test_block_size = 20;
	for (i = 0; i < w * h / _internal_test_block_size / _internal_test_block_size; i++) {
		color = (i & 0x1) * 0xff;
		color += ((i & 0x2) >> 1) * 0xff00;
		color += ((i & 0x4) >> 2) * 0xff0000;
		color += 0xff000000U;
		_mtkfb_draw_block(va, i % (w / _internal_test_block_size) * _internal_test_block_size,
				  i / (w / _internal_test_block_size) * _internal_test_block_size,
				  _internal_test_block_size, _internal_test_block_size, color);
	}
	primary_display_trigger(1, NULL, 0);
	_internal_test_block_size = 30;
	for (i = 0; i < w * h / _internal_test_block_size / _internal_test_block_size; i++) {
		color = (i & 0x1) * 0xff;
		color += ((i & 0x2) >> 1) * 0xff00;
		color += ((i & 0x4) >> 2) * 0xff0000;
		color += 0xff000000U;
		_mtkfb_draw_block(va, i % (w / _internal_test_block_size) * _internal_test_block_size,
				  i / (w / _internal_test_block_size) * _internal_test_block_size,
				  _internal_test_block_size, _internal_test_block_size, color);
	}
	primary_display_trigger(1, NULL, 0);

#if 0
	/* extern unsigned char data_rgb888_64x64[12288]; */

	/* memset_io(va, 0xff, w*h*4); */
	int i = 0;
	int j = 0;

	for (j = 0; j < 10; j++) {
		for (i = 0; i < 64; i++)
			memcpy((void *)(va + j * 720 * 70 * 3 + 720 * 3 * i),
			       data_rgb888_64x64 + 64 * 3 * i, 64 * 3);
	}


	for (j = 0; j < 10; j++) {
		for (i = 0; i < 64; i++)
			memcpy((void *)(va + 720 * 1280 * 3 + 360 * 3 + j * 720 * 70 * 3 +
					720 * 3 * i), data_rgb888_64x64 + 64 * 3 * i, 64 * 3);
	}

	primary_display_trigger(1);
	memset_io(va, 0xff, w * h * 4);
	primary_display_trigger(1);
	memset_io(va, 0x00, w * h * 4);
	primary_display_trigger(1);
	memset_io(va, 0x88, w * h * 4);
	primary_display_trigger(1);
	memset_io(va, 0xcc, w * h * 4);
	primary_display_trigger(1);
	memset_io(va, 0x22, w * h * 4);
	primary_display_trigger(1);
#endif
	return 0;
}
#endif

#ifdef CONFIG_OF
struct tag_videolfb {
	u64 fb_base;
	u32 islcmfound;
	u32 fps;
	u32 vram;
	char lcmname[1];	/* this is the minimum size */
};
unsigned int islcmconnected;
unsigned int vramsize;
phys_addr_t fb_base;
static int is_videofb_parse_done;
static unsigned long video_node;

static int fb_early_init_dt_get_chosen(unsigned long node, const char *uname, int depth, void *data)
{
	if (depth != 1 || (strcmp(uname, "chosen") != 0 && strcmp(uname, "chosen@0") != 0))
		return 0;

	video_node = node;
	return 1;
}

/* Retrun value: 0: success, 1: fail */
int _parse_tag_videolfb(void)
{
	struct tag_videolfb *videolfb_tag = NULL;
	/* not necessary */
	/* DISPCHECK("[DT][videolfb]isvideofb_parse_done = %d\n",is_videofb_parse_done); */

	if (is_videofb_parse_done)
		return 0;
#ifdef MTK_NO_DISP_IN_LK
	DISPCHECK("[DT][videolfb] zaikuo, workaround for LK not ready\n"); /* after LK ready, remove this code */
	return 1;
#endif

	if (of_scan_flat_dt(fb_early_init_dt_get_chosen, NULL) > 0) {
		videolfb_tag = (struct tag_videolfb *)of_get_flat_dt_prop(video_node, "atag,videolfb", NULL);
		if (videolfb_tag) {
			memset((void *)mtkfb_lcm_name, 0, sizeof(mtkfb_lcm_name));
			strcpy((char *)mtkfb_lcm_name, videolfb_tag->lcmname);
			mtkfb_lcm_name[strlen(videolfb_tag->lcmname)] = '\0';

			lcd_fps = videolfb_tag->fps;
			if (lcd_fps == 0)
				lcd_fps = 6000;

			islcmconnected = videolfb_tag->islcmfound;
			vramsize = videolfb_tag->vram;
			fb_base = videolfb_tag->fb_base;
			is_videofb_parse_done = 1;
			DISPPRINT("[DT][videolfb] lcmfound=%d, fps=%d, fb_base=%p, vram=%d, lcmname=%s\n",
			     islcmconnected, lcd_fps, (void *)fb_base, vramsize, mtkfb_lcm_name);
#if 0
			DISPCHECK("[DT][videolfb] islcmfound = %d\n", islcmconnected);
			DISPCHECK("[DT][videolfb] fps        = %d\n", lcd_fps);
			DISPCHECK("[DT][videolfb] fb_base    = %p\n", (void *)fb_base);
			DISPCHECK("[DT][videolfb] vram       = %d\n", vramsize);
			DISPCHECK("[DT][videolfb] lcmname    = %s\n", mtkfb_lcm_name);
#endif
			return 0;
		}

		DISPCHECK("[DT][videolfb] videolfb_tag not found\n");
		return 1;
	}

	DISPCHECK("[DT][videolfb] of_chosen not found\n");
	return 1;
}

phys_addr_t mtkfb_get_fb_base(void)
{
	_parse_tag_videolfb();
	return fb_base;
}
EXPORT_SYMBOL(mtkfb_get_fb_base);

size_t mtkfb_get_fb_size(void)
{
	_parse_tag_videolfb();
	return vramsize;
}
EXPORT_SYMBOL(mtkfb_get_fb_size);
#endif

#ifdef FPGA_DEBUG_PAN
static int update_test_kthread(void *data)
{
	/* struct sched_param param = { .sched_priority = RTPM_PRIO_SCRN_UPDATE }; */
	/* sched_setscheduler(current, SCHED_RR, &param); */
	unsigned int i = 0, j = 0;
	unsigned long fb_va;
	unsigned long fb_pa;
	unsigned int *fb_start;
	unsigned int fbsize = primary_display_get_height() * primary_display_get_width();

	mtkfb_fbi->var.yoffset = 0;
	disp_get_fb_address(&fb_va, &fb_pa);

	for (;;) {
		if (kthread_should_stop())
			break;
		msleep(1000); /* 2s */
		pr_debug("update test thread work,offset = %d\n", i);

		mtkfb_fbi->var.yoffset = 0;
		disp_get_fb_address(&fb_va, &fb_pa);
		fb_start = (unsigned int *)fb_va;
		for (j = 0; j < fbsize; j++) {
			*fb_start = (0x55) << ((i % 4) * 8);
			fb_start++;
		}
		mtkfb_pan_display_impl(&mtkfb_fbi->var, mtkfb_fbi);
		i++;
	}

	MTKFB_LOG("exit esd_recovery_kthread()\n");
	return 0;
}
#endif

static int mtkfb_probe(struct device *dev)
{
	struct platform_device *pdev;
	struct mtkfb_device *fbdev = NULL;
	struct fb_info *fbi;
	int init_state;
	int r = 0;
#ifdef DISP_GPIO_DTS
	long dts_gpio_state = 0;
#endif
	/* DISPFUNC(); */
	DISPPRINT("%s\n", __func__);

#ifdef CONFIG_OF
	_parse_tag_videolfb();
#else
	{
		char *p = NULL;

		pr_debug("%s, %s\n", __func__, saved_command_line);
		p = strstr(saved_command_line, "fps=");
		if (p == NULL) {
			lcd_fps = 6000;
			pr_debug("[FB driver]can not get fps from uboot\n");
		} else {
			p += 4;
			r = kstrtol(p, 10, &lcd_fps);
			if (r)
				pr_err("DISP/%s: errno %d\n", __func__, r);
			if (lcd_fps == 0)
				lcd_fps = 6000;
		}
	}
#endif

	init_state = 0;

	pdev = to_platform_device(dev);

#ifdef DISP_GPIO_DTS
	/* repo call DTS gpio module, if not necessary, invoke nothing */
	dts_gpio_state = disp_dts_gpio_init_repo(pdev);
	if (dts_gpio_state != 0)
		dev_err(&pdev->dev, "retrieve GPIO DTS failed.");
#endif

	fbi = framebuffer_alloc(sizeof(struct mtkfb_device), dev);
	if (!fbi) {
		DISPERR("unable to allocate memory for device info\n");
		r = -ENOMEM;
		goto cleanup;
	}

	fbdev = (struct mtkfb_device *)fbi->par;
	fbdev->fb_info = fbi;
	fbdev->dev = dev;
	dev_set_drvdata(dev, fbdev);

	{
#ifndef MTK_NO_DISP_IN_LK
#ifdef CONFIG_OF
		/* printk("mtkfb_probe:get FB MEM REG\n"); */
		_parse_tag_videolfb();
		/* printk("mtkfb_probe: fb_pa = %p\n",(void *)fb_base); */

		disp_hal_allocate_framebuffer(fb_base, (fb_base + vramsize - 1),
					      (unsigned long *)&fbdev->fb_va_base, &fb_pa);
		fbdev->fb_pa_base = fb_base;

#else
		struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

		vramsize = res->end - res->start + 1;
		/* ASSERT(DISP_GetVRamSize() <= (res->end - res->start + 1)); */
		disp_hal_allocate_framebuffer(res->start, res->end,
					      (unsigned int *)&fbdev->fb_va_base, &fb_pa);
		fbdev->fb_pa_base = res->start;
#endif
#else
		{
			struct resource res;

			unsigned long fb_mem_addr_pa = 0;
			unsigned long fb_mem_addr_va = 0;

			pr_debug("mtkfb_probe:get FB MEM REG\n");

			if (of_address_to_resource(dev->of_node, 0, &res) != 0) {
				r = -ENOMEM;
				goto cleanup;
			}
			fb_mem_addr_pa = res.start;

			fb_mem_addr_va = (unsigned long)of_iomap(dev->of_node, 0);

			pr_debug("mtkfb_probe: fb_pa = 0x%lx, fb_va = 0x%lx\n", fb_mem_addr_pa,
				 fb_mem_addr_va);

			disp_hal_allocate_framebuffer(res.start, res.end,
						      (unsigned int *)&fbdev->fb_va_base, &fb_pa);
			fbdev->fb_pa_base = res.start;
			fbdev->fb_va_base = fb_mem_addr_va;
		}
#endif
	}
	primary_display_set_frame_buffer_address((unsigned long)fbdev->fb_va_base, fb_pa);

	/* mtkfb should parse lcm name from kernel boot command line */
	primary_display_init(mtkfb_find_lcm_driver(), lcd_fps);

	init_state++; /* 1 */
	MTK_FB_XRES = primary_display_get_width();
	MTK_FB_YRES = primary_display_get_height();
	fb_xres_update = MTK_FB_XRES;
	fb_yres_update = MTK_FB_YRES;

	MTK_FB_BPP = primary_display_get_bpp();
	MTK_FB_PAGES = primary_display_get_pages();
	/* DISPMSG("MTK_FB_XRES=%d, MTKFB_YRES=%d, MTKFB_BPP=%d, MTK_FB_PAGES=%d, MTKFB_LINE=%d, MTKFB_SIZEV=%d\n",*/
		  /* MTK_FB_XRES, MTK_FB_YRES, MTK_FB_BPP, MTK_FB_PAGES, MTK_FB_LINE, MTK_FB_SIZEV); */
	fbdev->fb_size_in_byte = MTK_FB_SIZEV;

	/* Allocate and initialize video frame buffer */
	DISPPRINT("[FB Driver] fbdev->fb_pa_base = %p, fbdev->fb_va_base = %p\n",
		  &(fbdev->fb_pa_base), fbdev->fb_va_base);

	if (!fbdev->fb_va_base) {
		DISPERR("unable to allocate memory for frame buffer\n");
		r = -ENOMEM;
		goto cleanup;
	}
	init_state++; /* 2 */

	/* Register to system */

	r = mtkfb_fbinfo_init(fbi);
	if (r) {
		DISPERR("mtkfb_fbinfo_init fail, r = %d\n", r);
		goto cleanup;
	}
	init_state++; /* 4 */
	mtkfb_fbi = fbi;

	if (disp_helper_get_stage() == DISP_HELPER_STAGE_NORMAL) {
		/* dal_init should after mtkfb_fbinfo_init, otherwise layer 3 will show dal background color */
		DAL_STATUS ret;
		unsigned long fbVA = (unsigned long)fbdev->fb_va_base;
		unsigned long fbPA = fb_pa;
		/* DAL init here */
		fbVA += DISP_GetFBRamSize();
		fbPA += DISP_GetFBRamSize();
		ret = DAL_Init(fbVA, fbPA);
	}
#if 0	/* def FPGA_DEBUG_PAN */
	_mtkfb_internal_test(fbdev->fb_va_base, MTK_FB_XRES, MTK_FB_YRES);
#endif

	r = mtkfb_register_sysfs(fbdev);
	if (r) {
		DISPERR("mtkfb_register_sysfs fail, r = %d\n", r);
		goto cleanup;
	}
	init_state++; /* 5 */

	r = register_framebuffer(fbi);
	if (r != 0) {
		DISPERR("register_framebuffer failed\n");
		goto cleanup;
	}

	if (disp_helper_get_stage() != DISP_HELPER_STAGE_NORMAL)
		primary_display_diagnose();


	/* this function will get fb_heap base address to ion for management frame buffer */
	ion_drv_create_FB_heap(mtkfb_get_fb_base(), mtkfb_get_fb_size() - DAL_GetLayerSize());
	fbdev->state = MTKFB_ACTIVE;

#ifdef FPGA_DEBUG_PAN
#if 0
	{
		unsigned int cnt = 0;
		void *fb_va = (void *)fbdev->fb_va_base;
		unsigned int fbsize = primary_display_get_height() * primary_display_get_width();
#if 0
		for (cnt = 0; cnt < fbsize; cnt++)
			*(fb_va++) = 0xFFFF0000;
		for (cnt = 0; cnt < fbsize; cnt++)
			*(fb_va++) = 0xFF00FF00;
#else
		memset_io(fb_va, 0xFF, fbsize * 4);
		memset_io(fb_va + fbsize * 4, 0x55, fbsize * 4);
#endif
	}
	pr_debug("memset_io done\n");
#endif
	{
		struct task_struct *update_test_task = NULL;

		update_test_task = kthread_create(update_test_kthread, NULL, "update_test_kthread");

		if (IS_ERR(update_test_task))
			MTKFB_LOG("update test task create fail\n");
		else
			wake_up_process(update_test_task);
	}
#endif

	MSG_FUNC_LEAVE();
	return 0;

cleanup:
	mtkfb_free_resources(fbdev, init_state);

	/* printk("mtkfb_probe end\n"); */
	return r;
}

/* Called when the device is being detached from the driver */
static int mtkfb_remove(struct device *dev)
{
	struct mtkfb_device *fbdev = dev_get_drvdata(dev);
	enum mtkfb_state saved_state = fbdev->state;

	MSG_FUNC_ENTER();
	/* FIXME: wait till completion of pending events */

	fbdev->state = MTKFB_DISABLED;
	mtkfb_free_resources(fbdev, saved_state);

	MSG_FUNC_LEAVE();
	return 0;
}

/* PM suspend */
static int mtkfb_suspend(struct device *pdev, pm_message_t mesg)
{
	/* NOT_REFERENCED(pdev); */
	MSG_FUNC_ENTER();
	MTKFB_LOG("[FB Driver] mtkfb_suspend(): 0x%x\n", mesg.event);
	ovl2mem_wait_done();

	MSG_FUNC_LEAVE();
	return 0;
}

bool mtkfb_is_suspend(void)
{
	return primary_display_is_sleepd();
}
EXPORT_SYMBOL(mtkfb_is_suspend);

int mtkfb_ipoh_restore(struct notifier_block *nb, unsigned long val, void *ign)
{
	switch (val) {
	case PM_RESTORE_PREPARE:
		primary_display_ipoh_restore();
		pr_debug("[FB Driver] mtkfb_ipoh_restore PM_RESTORE_PREPARE\n");
		return NOTIFY_DONE;
	case PM_POST_RESTORE:
		primary_display_ipoh_recover();
		pr_debug("[FB Driver] %s pm_event: %lu\n", __func__, val);
		return NOTIFY_DONE;
	}

	return NOTIFY_OK;
}

int mtkfb_ipo_init(void)
{
	pm_nb.notifier_call = mtkfb_ipoh_restore;
	pm_nb.priority = 0;
	register_pm_notifier(&pm_nb);
	return 0;
}

static void mtkfb_shutdown(struct device *pdev)
{
	MTKFB_LOG("[FB Driver] mtkfb_shutdown()\n");
	/* mt65xx_leds_brightness_set(MT65XX_LED_TYPE_LCD, LED_OFF); */
	if (!lcd_fps)
		msleep(30);
	else
		msleep(2 * 100000 / lcd_fps);	/* Delay 2 frames. */

	if (primary_display_is_sleepd()) {
		MTKFB_LOG("mtkfb has been power off\n");
		return;
	}

	MTKFB_LOG("[FB Driver] cci400_sel_for_ddp\n");
	/* cci400_sel_for_ddp(); //FIXME:need confirm with Zhihui */

	primary_display_suspend();
	MTKFB_LOG("[FB Driver] leave mtkfb_shutdown\n");
}

void mtkfb_clear_lcm(void)
{
#if 0
	int i;
	unsigned int layer_status[DDP_OVL_LAYER_MUN] = { 0 };

	mutex_lock(&OverlaySettingMutex);
	for (i = 0; i < DDP_OVL_LAYER_MUN; i++) {
		layer_status[i] = cached_layer_config[i].layer_en;
		cached_layer_config[i].layer_en = 0;
		cached_layer_config[i].isDirty = 1;
	}
	atomic_set(&OverlaySettingDirtyFlag, 1);
	atomic_set(&OverlaySettingApplied, 0);
	mutex_unlock(&OverlaySettingMutex);

	/* DISP_CHECK_RET(DISP_UpdateScreen(0, 0, fb_xres_update, fb_yres_update)); */
	/* DISP_CHECK_RET(DISP_UpdateScreen(0, 0, fb_xres_update, fb_yres_update)); */
	/* DISP_WaitForLCDNotBusy(); */
	mutex_lock(&OverlaySettingMutex);
	for (i = 0; i < DDP_OVL_LAYER_MUN; i++) {
		cached_layer_config[i].layer_en = layer_status[i];
		cached_layer_config[i].isDirty = 1;
	}
	atomic_set(&OverlaySettingDirtyFlag, 1);
	atomic_set(&OverlaySettingApplied, 0);
	mutex_unlock(&OverlaySettingMutex);
#endif
}


static int mtkfb_blank_suspend(void)
{
	int ret = 0;

	MSG_FUNC_ENTER();

	if (disp_helper_get_stage() != DISP_HELPER_STAGE_NORMAL)
		return ret;

#ifdef CONFIG_SINGLE_PANEL_OUTPUT
	is_early_suspended = true;
#endif

	pr_debug("[FB Driver] enter early_suspend\n");
#ifdef CONFIG_MTK_LEDS
/* mt65xx_leds_brightness_set(MT65XX_LED_TYPE_LCD, LED_OFF); */
#endif
	msleep(30);

	ret = primary_display_suspend();

	if (ret < 0) {
		DISPERR("primary display suspend failed\n");
		return ret;
	}
	pr_debug("[FB Driver] leave early_suspend\n");

	return ret;
}

/* PM resume */
static int mtkfb_resume(struct device *pdev)
{
	/* NOT_REFERENCED(pdev); */
	MSG_FUNC_ENTER();
	MTKFB_LOG("[FB Driver] mtkfb_resume()\n");
	MSG_FUNC_LEAVE();
	return 0;
}

static int mtkfb_blank_resume(void)
{
	int ret = 0;

	MSG_FUNC_ENTER();

	if (disp_helper_get_stage() != DISP_HELPER_STAGE_NORMAL)
		return ret;

	pr_debug("[FB Driver] enter late_resume\n");

#ifdef CONFIG_SINGLE_PANEL_OUTPUT
	is_early_suspended = false;
#endif

	ret = primary_display_resume();

	if (ret) {
		DISPERR("primary display resume failed\n");
		return ret;
	}

	pr_debug("[FB Driver] leave late_resume\n");

	return ret;
}

/*---------------------------------------------------------------------------*/
#ifdef CONFIG_PM
/*---------------------------------------------------------------------------*/
int mtkfb_pm_suspend(struct device *device)
{
	/* pr_debug("calling %s()\n", __func__); */
	struct platform_device *pdev = to_platform_device(device);

	if (pdev == NULL) {
		DISPERR("pm suspend fail\n");
		ASSERT(0);
		return -1;
	}

	return mtkfb_suspend((struct device *)pdev, PMSG_SUSPEND);
}

int mtkfb_pm_resume(struct device *device)
{
	/* pr_debug("calling %s()\n", __func__); */
	struct platform_device *pdev = to_platform_device(device);

	if (pdev == NULL) {

		DISPERR("pm suspend fail\n");
		ASSERT(0);
		return -1;
	}


	return mtkfb_resume((struct device *)pdev);
}

int mtkfb_pm_freeze(struct device *device)
{
	primary_display_esd_check_enable(0);
	return 0;
}

int mtkfb_pm_restore_noirq(struct device *device)
{
	DISPCHECK("%s: %d\n", __func__, __LINE__);

	is_ipoh_bootup = true;
#ifndef CONFIG_MTK_CLKMGR
	dpmgr_path_power_on(primary_get_dpmgr_handle(), CMDQ_DISABLE);
#endif
	return 0;
}

/*---------------------------------------------------------------------------*/
#else				/*CONFIG_PM */
/*---------------------------------------------------------------------------*/
#define mtkfb_pm_suspend NULL
#define mtkfb_pm_resume  NULL
#define mtkfb_pm_restore_noirq NULL
#define mtkfb_pm_freeze NULL
/*---------------------------------------------------------------------------*/
#endif				/*CONFIG_PM */
/*---------------------------------------------------------------------------*/
static const struct of_device_id mtkfb_of_ids[] = {
	{.compatible = "mediatek,mtkfb",},
	{}
};

const struct dev_pm_ops mtkfb_pm_ops = {
	.suspend = mtkfb_pm_suspend,
	.resume = mtkfb_pm_resume,
	.freeze = mtkfb_pm_freeze,
	.thaw = mtkfb_pm_resume,
	.poweroff = mtkfb_pm_suspend,
	.restore = mtkfb_pm_resume,
	.restore_noirq = mtkfb_pm_restore_noirq,
};

static struct platform_driver mtkfb_driver = {
	.driver = {
		.name = MTKFB_DRIVER,
#ifdef CONFIG_PM
		.pm = &mtkfb_pm_ops,
#endif
		.bus = &platform_bus_type,
		.probe = mtkfb_probe,
		.remove = mtkfb_remove,
		.suspend = mtkfb_suspend,
		.resume = mtkfb_resume,
		.shutdown = mtkfb_shutdown,
		.of_match_table = mtkfb_of_ids,
	},
};

#if 0
#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend mtkfb_early_suspend_handler = {
	.level = EARLY_SUSPEND_LEVEL_DISABLE_FB,
	.suspend = mtkfb_early_suspend,
	.resume = mtkfb_late_resume,
};
#endif
#endif


int mtkfb_get_debug_state(char *stringbuf, int buf_len)
{
	int len = 0;
	struct mtkfb_device *fbdev = (struct mtkfb_device *)mtkfb_fbi->par;

	unsigned long va = (unsigned long)fbdev->fb_va_base;
	unsigned long mva = (unsigned long)fbdev->fb_pa_base;
	unsigned long pa = fbdev->fb_pa_base;
	unsigned int resv_size = vramsize;

	len += scnprintf(stringbuf + len, buf_len - len,
		      "|--------------------------------------------------------------------------------------|\n");
	/* len += scnprintf(stringbuf+len, buf_len - len, "********MTKFB Driver General Information********\n"); */
	len += scnprintf(stringbuf + len, buf_len - len,
		      "|Framebuffer VA:0x%lx, PA:0x%lx, MVA:0x%lx, Reserved Size:0x%08x|%d\n", va,
		      pa, mva, resv_size, resv_size);
	len += scnprintf(stringbuf + len, buf_len - len, "|xoffset=%d, yoffset=%d\n",
		      mtkfb_fbi->var.xoffset, mtkfb_fbi->var.yoffset);
	len += scnprintf(stringbuf + len, buf_len - len, "|framebuffer line alignment(for gpu)=%d\n",
		      MTK_FB_ALIGNMENT);
	len += scnprintf(stringbuf + len, buf_len - len,
		      "|xres=%d, yres=%d,bpp=%d,pages=%d,linebytes=%d,total size=%d\n", MTK_FB_XRES,
		      MTK_FB_YRES, MTK_FB_BPP, MTK_FB_PAGES, MTK_FB_LINE, MTK_FB_SIZEV);
	/* use extern in case DAL_LOCK is hold, then can't get any debug info */
	len += scnprintf(stringbuf + len, buf_len - len, "|AEE Layer is %s\n",
		      isAEEEnabled ? "enabled" : "disabled");

	return len;
}


/* Register both the driver and the device */
int __init mtkfb_init(void)
{
	int r = 0;

	MSG_FUNC_ENTER();

	if (platform_driver_register(&mtkfb_driver)) {
		PRNERR("failed to register mtkfb driver\n");
		r = -ENODEV;
		goto exit;
	}
#if 0
#ifdef CONFIG_HAS_EARLYSUSPEND
	register_early_suspend(&mtkfb_early_suspend_handler);
#endif
#endif
	/* FIXME: find definition */
	PanelMaster_Init();

	DBG_Init();
	mtkfb_ipo_init();
exit:
	MSG_FUNC_LEAVE();
	return r;
}


static void __exit mtkfb_cleanup(void)
{
	MSG_FUNC_ENTER();

	platform_driver_unregister(&mtkfb_driver);

#if 0
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&mtkfb_early_suspend_handler);
#endif
#endif

	/* FIXME: find definition of PanelMaster_Deinit */
	/* PanelMaster_Deinit(); */

	DBG_Deinit();

	MSG_FUNC_LEAVE();
}
module_init(mtkfb_init);
module_exit(mtkfb_cleanup);
MODULE_DESCRIPTION("MEDIATEK framebuffer driver");
MODULE_AUTHOR("Xuecheng Zhang <Xuecheng.Zhang@mediatek.com>");
MODULE_LICENSE("GPL");

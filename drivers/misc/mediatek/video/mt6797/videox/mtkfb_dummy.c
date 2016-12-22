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

#include <generated/autoconf.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/earlysuspend.h>
#include <linux/kthread.h>
#include <linux/vmalloc.h>
#include <linux/disp_assert_layer.h>
#include <linux/semaphore.h>
#include <linux/xlog.h>
#include <linux/mutex.h>
#include <linux/leds-mt65xx.h>
#include <linux/suspend.h>
#include <linux/of_fdt.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/dma-buf.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/cacheflush.h>
#include <asm/io.h>
#include <mach/dma.h>
#include <mach/irqs.h>
#include <linux/dma-mapping.h>
#include "mach/mt_boot.h"
#include "mtkfb.h"
#include "mtkfb_info.h"

#define DISPCHECK pr_debug
#define ALIGN_TO(x, n)  \
	(((x) + ((n) - 1)) & ~((n) - 1))

#define SCREEN_WIDHT  (1080)
#define SCREEN_HEIGHT  (1920)

#define MTK_FB_ALIGNMENT 32

static u32 MTK_FB_XRES;
static u32 MTK_FB_YRES;
static u32 MTK_FB_BPP;
static u32 MTK_FB_PAGES;
static u32 fb_xres_update;
static u32 fb_yres_update;
static unsigned long size;
#define MTK_FB_XRESV (ALIGN_TO(MTK_FB_XRES, MTK_FB_ALIGNMENT))
#define MTK_FB_YRESV (ALIGN_TO(MTK_FB_YRES, MTK_FB_ALIGNMENT) * MTK_FB_PAGES)	/* For page flipping */
#define MTK_FB_BYPP  ((MTK_FB_BPP + 7) >> 3)
#define MTK_FB_LINE  (ALIGN_TO(MTK_FB_XRES, MTK_FB_ALIGNMENT) * MTK_FB_BYPP)
#define MTK_FB_SIZE  (MTK_FB_LINE * ALIGN_TO(MTK_FB_YRES, MTK_FB_ALIGNMENT))

#define MTK_FB_SIZEV (MTK_FB_LINE * ALIGN_TO(MTK_FB_YRES, MTK_FB_ALIGNMENT) * MTK_FB_PAGES)


/* --------------------------------------------------------------------------- */
/* local variables */
/* --------------------------------------------------------------------------- */

unsigned long fb_pa = 0;
struct fb_info *mtkfb_fbi;
unsigned int lcd_fps = 6000;
char mtkfb_lcm_name[256] = { 0 };

bool is_ipoh_bootup = false;
BOOL is_early_suspended = FALSE;


/* --------------------------------------------------------------------------- */
/* local function declarations */
/* --------------------------------------------------------------------------- */

#ifdef CONFIG_OF
static int _parse_tag_videolfb(void);
#endif


int mtkfb_get_debug_state(char *stringbuf, int buf_len)
{
	return 0;
}

unsigned int mtkfb_fm_auto_test(void)
{
	return 0;
}


/* Init frame buffer content as 3 R/G/B color bars for debug */
static int init_framebuffer(struct fb_info *info)
{
	void *buffer = info->screen_base + info->var.yoffset * info->fix.line_length;

	/* clean whole frame buffer as black */
	memset(buffer, 0, info->screen_size);

	return 0;
}

unsigned int DISP_GetVRamSizeBoot(void)
{
	return 0x01800000;
}

/*
 * ---------------------------------------------------------------------------
 * fbdev framework callbacks and the ioctl interface
 * ---------------------------------------------------------------------------
 */
/* Called each time the mtkfb device is opened */
static int mtkfb_open(struct fb_info *info, int user)
{
	return 0;
}

/* Called when the mtkfb device is closed. We make sure that any pending
 * gfx DMA operations are ended, before we return. */
static int mtkfb_release(struct fb_info *info, int user)
{
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

	NOT_REFERENCED(transp);
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
	return r;
}


/* Set fb_info.fix fields and also updates fbdev.
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


/* Switch to a new mode. The parameters for it has been check already by
 * mtkfb_check_var.
 */
static int mtkfb_set_par(struct fb_info *fbi)
{
	struct fb_var_screeninfo *var = &fbi->var;
	struct mtkfb_device *fbdev = (struct mtkfb_device *)fbi->par;
	set_fb_fix(fbdev);
	return 0;
}

static int mtkfb_pan_display_impl(struct fb_var_screeninfo *var, struct fb_info *info)
{

	pr_debug("xoffset=%u, yoffset=%u, xres=%u, yres=%u, xresv=%u, yresv=%u\n", var->xoffset,
	       var->yoffset, info->var.xres, info->var.yres, info->var.xres_virtual,
	       info->var.yres_virtual);
	info->var.yoffset = var->yoffset;
	return 0;
}


/* Check values in var, try to adjust them in case of out of bound values if
 * possible, or return error.
 */
static int mtkfb_check_var(struct fb_var_screeninfo *var, struct fb_info *fbi)
{
	unsigned int bpp;
	unsigned long max_frame_size;
	unsigned long line_size;

	struct mtkfb_device *fbdev = (struct mtkfb_device *)fbi->par;

	/* DISPFUNC(); */

	pr_debug("mtkfb_check_var,xres=%u,yres=%u,x_vir=%u,y_vir=%u,xoffset=%u,yoffset=%u,bits_per_pixel=%u)\n",
	       var->xres, var->yres, var->xres_virtual, var->yres_virtual,
	       var->xoffset, var->yoffset, var->bits_per_pixel);

	bpp = var->bits_per_pixel;

	if (bpp != 16 && bpp != 24 && bpp != 32) {
		pr_debug("[%s]unsupported bpp: %d", __func__, bpp);
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
	pr_debug("fbdev->fb_size_in_byte=0x%08lx\n", fbdev->fb_size_in_byte);
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
	pr_error("mtkfb_check_var,xres=%u,yres=%u,x_vir=%u,y_vir=%u,xoffset=%u,yoffset=%u,bits_per_pixel=%u)\n",
	       var->xres, var->yres, var->xres_virtual, var->yres_virtual,
	       var->xoffset, var->yoffset, var->bits_per_pixel);
	if (var->xres + var->xoffset > var->xres_virtual)
		var->xoffset = var->xres_virtual - var->xres;
	if (var->yres + var->yoffset > var->yres_virtual)
		var->yoffset = var->yres_virtual - var->yres;

	pr_error("mtkfb_check_var,xres=%u,yres=%u,x_vir=%u,y_vir=%u,xoffset=%u,yoffset=%u,bits_per_pixel=%u)\n",
	       var->xres, var->yres, var->xres_virtual, var->yres_virtual,
	       var->xoffset, var->yoffset, var->bits_per_pixel);

	if (16 == bpp) {
		var->red.offset = 11;
		var->red.length = 5;
		var->green.offset = 5;
		var->green.length = 6;
		var->blue.offset = 0;
		var->blue.length = 5;
		var->transp.offset = 0;
		var->transp.length = 0;
	} else if (24 == bpp) {
		var->red.length = var->green.length = var->blue.length = 8;
		var->transp.length = 0;

		/* Check if format is RGB565 or BGR565 */

		ASSERT(8 == var->green.offset);
		ASSERT(16 == var->red.offset + var->blue.offset);
		ASSERT(16 == var->red.offset || 0 == var->red.offset);
	} else if (32 == bpp) {
		var->red.length = var->green.length = var->blue.length = var->transp.length = 8;

		/* Check if format is ARGB565 or ABGR565 */

		ASSERT(8 == var->green.offset && 24 == var->transp.offset);
		ASSERT(16 == var->red.offset + var->blue.offset);
		ASSERT(16 == var->red.offset || 0 == var->red.offset);
	}

	var->red.msb_right = var->green.msb_right = var->blue.msb_right = var->transp.msb_right = 0;

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

	return 0;
}


static int mtkfb_pan_display_proxy(struct fb_var_screeninfo *var, struct fb_info *info)
{
	return mtkfb_pan_display_impl(var, info);
}

#ifdef CONFIG_DMA_SHARED_BUFFER
static struct sg_table *mtkfb_dma_buf_map(struct dma_buf_attachment *attachment,
					  enum dma_data_direction dir)
{
	struct sg_table *table;
	struct fb_info *info = attachment->dmabuf->priv;
	struct page *page;
	int err;

	table = kmalloc(sizeof(struct sg_table), GFP_KERNEL);
	if (!table)
		return ERR_PTR(-ENOMEM);

	err = sg_alloc_table(table, 1, GFP_KERNEL);
	if (err) {
		kfree(table);
		return ERR_PTR(err);
	}

	sg_set_page(table->sgl, NULL, info->fix.smem_len, 0);

	sg_dma_address(table->sgl) = info->fix.smem_start;
	debug_dma_map_sg(NULL, table->sgl, table->nents, table->nents, DMA_BIDIRECTIONAL);

	return table;
}

static void mtkfb_dma_buf_unmap(struct dma_buf_attachment *attachment, struct sg_table *table,
				enum dma_data_direction dir)
{
	debug_dma_unmap_sg(NULL, table->sgl, table->nents, DMA_BIDIRECTIONAL);
	sg_free_table(table);
	kfree(table);
}

static void mtkfb_dma_buf_release(struct dma_buf *buf)
{
	/* Nop */
}

static void *mtkfb_dma_buf_kmap_atomic(struct dma_buf *buf, unsigned long off)
{
	/* Not supported */
	return NULL;
}

static void *mtkfb_dma_buf_kmap(struct dma_buf *buf, unsigned long off)
{
	/* Not supported */
	return NULL;
}

static int mtkfb_dma_buf_mmap(struct dma_buf *buf, struct vm_area_struct *vma)
{
	struct fb_info *info = (struct fb_info *)buf->priv;

	pr_error(KERN_INFO "mtkfb: mmap dma-buf: %p, start: %lu, offset: %lu, size: %lu\n",
	       buf, vma->vm_start, vma->vm_pgoff, vma->vm_end - vma->vm_start);

	return io_remap_pfn_range(vma,
				  vma->vm_start + vma->vm_pgoff,
				  (info->fix.smem_start + vma->vm_pgoff) >> PAGE_SHIFT,
				  vma->vm_end - vma->vm_start,
				  pgprot_writecombine(vma->vm_page_prot));
}

static struct dma_buf_ops mtkfb_dma_buf_ops = {
	.map_dma_buf = mtkfb_dma_buf_map,
	.unmap_dma_buf = mtkfb_dma_buf_unmap,
	.release = mtkfb_dma_buf_release,
	.kmap_atomic = mtkfb_dma_buf_kmap_atomic,
	.kmap = mtkfb_dma_buf_kmap,
	.mmap = mtkfb_dma_buf_mmap,
};

static struct dma_buf *mtkfb_dmabuf_export(struct fb_info *info)
{
	struct dma_buf *buf;

	pr_error(KERN_INFO "mtkfb: Exporting dma-buf\n");


	buf = dma_buf_export(info, &mtkfb_dma_buf_ops, info->fix.smem_len, O_RDWR);

	return buf;
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
	.fb_cursor = NULL,
	.fb_check_var = mtkfb_check_var,
	.fb_set_par = mtkfb_set_par,
	.fb_ioctl = NULL,
#ifdef CONFIG_DMA_SHARED_BUFFER
	.fb_dmabuf_export = mtkfb_dmabuf_export,
#endif
};


/*
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

	BUG_ON(!fbdev->fb_va_base);
	info->fbops = &mtkfb_ops;
	info->flags = FBINFO_FLAG_DEFAULT;
	info->screen_base = (char *)fbdev->fb_va_base;
	info->screen_size = fbdev->fb_size_in_byte;
	info->pseudo_palette = fbdev->pseudo_palette;

	r = fb_alloc_cmap(&info->cmap, 32, 0);
	if (r != 0)
		pr_error("unable to allocate color map memory\n");

	/* setup the initial video mode (RGB565) */

	memset(&var, 0, sizeof(var));

	var.xres = MTK_FB_XRES;
	var.yres = MTK_FB_YRES;
	var.xres_virtual = MTK_FB_XRESV;
	var.yres_virtual = MTK_FB_YRESV;
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

	var.width = 0;
	var.height = 0;

	var.activate = FB_ACTIVATE_NOW;

	r = mtkfb_check_var(&var, info);
	if (r != 0)
		pr_error("failed to mtkfb_check_var\n");

	info->var = var;

	r = mtkfb_set_par(info);
	if (r != 0)
		pr_error("failed to mtkfb_set_par\n");
	return r;
}

/* Release the fb_info object */
static void mtkfb_fbinfo_cleanup(struct mtkfb_device *fbdev)
{
	fb_dealloc_cmap(&fbdev->fb_info->cmap);
}

/* Free driver resources. Can be called to rollback an aborted initialization
 * sequence.
 */
static void mtkfb_free_resources(struct mtkfb_device *fbdev, int state)
{
	int r = 0;

	switch (state) {
	case MTKFB_ACTIVE:
		r = unregister_framebuffer(fbdev->fb_info);
		ASSERT(0 == r);
		/* lint -fallthrough */
	case 4:
		mtkfb_fbinfo_cleanup(fbdev);
		/* lint -fallthrough */
	case 2:
#ifndef FPGA_EARLY_PORTING
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
		BUG();
	}
}

UINT32 DISP_GetScreenWidth(void)
{
	return SCREEN_WIDHT;
}

UINT32 DISP_GetScreenHeight(void)
{
	return SCREEN_HEIGHT;
}

void disp_get_fb_address(unsigned long *fbVirAddr, unsigned long *fbPhysAddr)
{
	struct mtkfb_device *fbdev = (struct mtkfb_device *)mtkfb_fbi->par;

	*fbVirAddr =
	    (unsigned long)fbdev->fb_va_base + mtkfb_fbi->var.yoffset * mtkfb_fbi->fix.line_length;
	*fbPhysAddr =
	    (unsigned long)fbdev->fb_pa_base + mtkfb_fbi->var.yoffset * mtkfb_fbi->fix.line_length;
}

static int mtkfb_fbinfo_modify(struct fb_info *info)
{
	struct fb_var_screeninfo var;
	int r = 0;

	memcpy(&var, &(info->var), sizeof(var));
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
	var.yoffset = var.yres;

	r = mtkfb_check_var(&var, info);
	if (r != 0)
		pr_error("failed to mtkfb_check_var\n");

	info->var = var;

	r = mtkfb_set_par(info);
	if (r != 0)
		pr_error("failed to mtkfb_set_par\n");

	return r;
}

unsigned int vramsize = 0;
#ifdef CONFIG_OF
struct tag_videolfb {
	u64 fb_base;
	u32 islcmfound;
	u32 fps;
	u32 vram;
	char lcmname[1];	/* this is the minimum size */
};
unsigned int islcmconnected = 0;
phys_addr_t fb_base = 0;
static int is_videofb_parse_done;
static unsigned long video_node;

static int fb_early_init_dt_get_chosen(unsigned long node, const char *uname, int depth, void *data)
{
	if (depth != 1 || (strcmp(uname, "chosen") != 0 && strcmp(uname, "chosen@0") != 0))
		return 0;
	video_node = node;
	return 1;
}

/* 0: success / 1: fail */
static int _parse_tag_videolfb(void)
{
	unsigned long size = 0;
	void *prop = NULL;
	struct tag_videolfb *videolfb_tag = NULL;

	pr_error("[DT][videolfb]isvideofb_parse_done = %d\n", is_videofb_parse_done);

	if (is_videofb_parse_done)
		return;
	if (of_scan_flat_dt(fb_early_init_dt_get_chosen, NULL) > 0) {
		videolfb_tag =
		    (struct tag_videolfb *)of_get_flat_dt_prop(video_node, "atag,videolfb", NULL);
		if (videolfb_tag) {
			pr_error("[DT][videolfb] find video info from lk\n");
			memset((void *)mtkfb_lcm_name, 0, sizeof(mtkfb_lcm_name));
			strcpy((char *)mtkfb_lcm_name, videolfb_tag->lcmname);
			mtkfb_lcm_name[strlen(videolfb_tag->lcmname)] = '\0';

			lcd_fps = videolfb_tag->fps;
			if (0 == lcd_fps)
				lcd_fps = 6000;

			islcmconnected = videolfb_tag->islcmfound;
			vramsize = videolfb_tag->vram;
			fb_base = videolfb_tag->fb_base;
			is_videofb_parse_done = 1;
			pr_error("[DT][videolfb] islcmfound = %d\n", islcmconnected);
			pr_error("[DT][videolfb] fps        = %d\n", lcd_fps);
			pr_error("[DT][videolfb] fb_base    = 0x%p\n", (void *)fb_base);
			pr_error("[DT][videolfb] vram       = %d\n", vramsize);
			pr_error("[DT][videolfb] lcmname    = %s\n", mtkfb_lcm_name);
			return 0;
		} else {
			pr_error("[DT][videolfb] find video info from dts\n");
			prop = of_get_flat_dt_prop(video_node, "atag,videolfb-fb_base", NULL);
			if (!prop) {
				pr_error("[DT][videolfb] fail to parse fb_base\n");
				return -1;
			}
			fb_base = of_read_number(prop, 1);

			prop = of_get_flat_dt_prop(video_node, "atag,videolfb-islcmfound", NULL);
			if (!prop) {
				pr_error("[DT][videolfb] fail to parse islcmfound\n");
				return -1;
			}
			islcmconnected = of_read_number(prop, 1);

			prop = of_get_flat_dt_prop(video_node, "atag,videolfb-fps", NULL);
			if (!prop) {
				pr_error("[DT][videolfb] fail to parse fps\n");
				return -1;
			}
			lcd_fps = of_read_number(prop, 1);
			if (0 == lcd_fps)
				lcd_fps = 6000;

			prop = of_get_flat_dt_prop(video_node, "atag,videolfb-vramSize", NULL);
			if (!prop) {
				pr_error("[DT][videolfb] fail to parse vramSize\n");
				return -1;
			}
			vramsize = of_read_number(prop, 1);

			prop = of_get_flat_dt_prop(video_node, "atag,videolfb-lcmname", &size);
			if (!prop) {
				pr_error("[DT][videolfb] fail to parse lcmname\n");
				return -1;
			}

			if (size >= sizeof(mtkfb_lcm_name)) {
				DISPCHECK("%s: error to get lcmname size=%ld\n", __func__,
					  size);
				return -1;
			}
			memset((void *)mtkfb_lcm_name, 0, sizeof(mtkfb_lcm_name));
			strncpy((char *)mtkfb_lcm_name, prop, sizeof(mtkfb_lcm_name));
			mtkfb_lcm_name[size] = '\0';

			is_videofb_parse_done = 1;

			pr_error("[DT][videolfb] fb_base    = %p\n", (void *)fb_base);
			pr_error("[DT][videolfb] islcmfound = %d\n", islcmconnected);
			pr_error("[DT][videolfb] fps        = %d\n", lcd_fps);
			pr_error("[DT][videolfb] vram       = %d\n", vramsize);
			pr_error("[DT][videolfb] lcmname    = %s\n", mtkfb_lcm_name);
			return 0;
		}
	} else {
		pr_error("[DT][videolfb] of_chosen not found\n");
		return 1;
	}
}

phys_addr_t mtkfb_get_fb_base(void)
{
	_parse_tag_videolfb();
	return fb_base;
}
#endif


int mtkfb_allocate_framebuffer(phys_addr_t pa_start, phys_addr_t pa_end, unsigned long *va,
			       unsigned long *mva)
{
	int ret = 0;
	*va = (unsigned long)ioremap_nocache(pa_start, pa_end - pa_start + 1);
	pr_error("disphal_allocate_fb, pa=%p, va=0x%lx\n", (void *)pa_start, *va);
	{
		*mva = pa_start & 0xffffffffULL;
	}
	return 0;
}

static int mtkfb_probe(struct device *dev)
{
	struct platform_device *pdev;
	struct mtkfb_device *fbdev = NULL;
	struct fb_info *fbi;
	int init_state;
	int r = 0;
	char *p = NULL;

	pr_error("mtkfb_probe begin\n");

#ifdef CONFIG_OF
	_parse_tag_videolfb();
#else
	pr_error("%s, no device tree ??!!\n", __func__);
	BUG();
#endif
	init_state = 0;

	pdev = to_platform_device(dev);

	fbi = framebuffer_alloc(sizeof(struct mtkfb_device), dev);
	if (!fbi) {
		pr_error("unable to allocate memory for device info\n");
		r = -ENOMEM;
		goto cleanup;
	}

	fbdev = (struct mtkfb_device *)fbi->par;
	fbdev->fb_info = fbi;
	fbdev->dev = dev;
	dev_set_drvdata(dev, fbdev);

	{

#ifdef CONFIG_OF
		pr_error("mtkfb_probe:get FB MEM REG\n");
		_parse_tag_videolfb();
		pr_error("mtkfb_probe: fb_pa = %p\n", (void *)fb_base);

		mtkfb_allocate_framebuffer(fb_base, (fb_base + vramsize - 1),
					   (unsigned int *)&fbdev->fb_va_base, &fb_pa);
		fbdev->fb_pa_base = fb_base;
#else
		struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		disp_hal_allocate_framebuffer(res->start, res->end,
					      (unsigned int *)&fbdev->fb_va_base, &fb_pa);
		fbdev->fb_pa_base = res->start;
#endif
	}

	init_state++;		/* 1 */
	MTK_FB_XRES = SCREEN_WIDHT;
	MTK_FB_YRES = SCREEN_HEIGHT;
	fb_xres_update = MTK_FB_XRES;
	fb_yres_update = MTK_FB_YRES;

	MTK_FB_BPP = 32;
	MTK_FB_PAGES = 3;
	pr_error
	    ("MTK_FB_XRES=%d, MTKFB_YRES=%d, MTKFB_BPP=%d, MTK_FB_PAGES=%d, MTKFB_LINE=%d, MTKFB_SIZEV=%d\n",
	     MTK_FB_XRES, MTK_FB_YRES, MTK_FB_BPP, MTK_FB_PAGES, MTK_FB_LINE, MTK_FB_SIZEV);
	fbdev->fb_size_in_byte = MTK_FB_SIZEV;

	/* Allocate and initialize video frame buffer */
	pr_error("[FB Driver] fbdev->fb_pa_base = %lx, fbdev->fb_va_base = %lx\n",
	       (unsigned long)fbdev->fb_pa_base, (unsigned long)(fbdev->fb_va_base));

	if (!fbdev->fb_va_base) {
		pr_error("unable to allocate memory for frame buffer\n");
		r = -ENOMEM;
		goto cleanup;
	}
	init_state++;		/* 2 */

	/* Register to system */

	r = mtkfb_fbinfo_init(fbi);
	if (r) {
		pr_error("mtkfb_fbinfo_init fail, r = %d\n", r);
		goto cleanup;
	}
	init_state++;		/* 4 */
	mtkfb_fbi = fbi;
	init_state++;		/* 5 */

	r = register_framebuffer(fbi);
	if (r != 0) {
		pr_error("register_framebuffer failed\n");
		goto cleanup;
	}
	fbdev->state = MTKFB_ACTIVE;
	pr_error("mtkfb_probe end\n");
	return 0;

cleanup:
	mtkfb_free_resources(fbdev, init_state);

	pr_error("mtkfb_probe end\n");
	return r;
}


/* Called when the device is being detached from the driver */
static int mtkfb_remove(struct device *dev)
{
	struct mtkfb_device *fbdev = dev_get_drvdata(dev);
	enum mtkfb_state saved_state = fbdev->state;
	fbdev->state = MTKFB_DISABLED;
	mtkfb_free_resources(fbdev, saved_state);
	return 0;
}

int mtkfb_ipo_init(void)
{
	return 0;
}

static void mtkfb_shutdown(struct device *pdev)
{
}

void mtkfb_clear_lcm(void)
{
}

static const struct of_device_id mtkfb_of_ids[] = {
	{.compatible = "mediatek,MTKFB",},
	{}
};

static struct platform_driver mtkfb_driver = {
	.driver = {
		   .name = MTKFB_DRIVER,
#ifdef CONFIG_PM
		   .pm = NULL,
#endif
		   .bus = &platform_bus_type,
		   .probe = mtkfb_probe,
		   .remove = mtkfb_remove,
		   .suspend = NULL,
		   .resume = NULL,
		   .shutdown = NULL,
		   .of_match_table = mtkfb_of_ids,
		   },
};

/* Register both the driver and the device */
int __init mtkfb_init(void)
{
	int r = 0;

	pr_error("mtkfb_init init");

	if (platform_driver_register(&mtkfb_driver)) {
		pr_error("failed to register mtkfb driver\n");
		r = -ENODEV;
	}
	return r;
}

static void __exit mtkfb_cleanup(void)
{
	platform_driver_unregister(&mtkfb_driver);

}


module_init(mtkfb_init);
module_exit(mtkfb_cleanup);

MODULE_DESCRIPTION("MEDIATEK framebuffer driver");
MODULE_AUTHOR("Xuecheng Zhang <Xuecheng.Zhang@mediatek.com>");
MODULE_LICENSE("GPL");

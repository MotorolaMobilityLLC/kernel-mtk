/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author: CK Hu <ck.hu@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <drm/drmP.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_plane_helper.h>
#include <linux/dma-buf.h>
#include <linux/reservation.h>

#include "mtk_drm_crtc.h"
#include "mtk_drm_ddp_comp.h"
#include "mtk_drm_drv.h"
#include "mtk_drm_fb.h"
#include "mtk_drm_gem.h"
#include "mtk_drm_plane.h"

static const uint32_t formats[] = {
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_RGB565,
};

static void mtk_plane_config(struct mtk_drm_plane *mtk_plane, bool enable,
			     dma_addr_t addr, struct drm_rect *dest)
{
	struct drm_plane *plane = &mtk_plane->base;
	struct mtk_plane_state *state = to_mtk_plane_state(plane->state);
	unsigned int pitch, format;
	int x, y;

	if (WARN_ON(!plane->state || (enable && !plane->state->fb)))
		return;

	if (plane->state->fb) {
		pitch = plane->state->fb->pitches[0];
		format = plane->state->fb->pixel_format;
	} else {
		pitch = 0;
		format = DRM_FORMAT_RGBA8888;
	}

	x = plane->state->crtc_x;
	y = plane->state->crtc_y;

	if (x < 0) {
		addr -= x * 4;
		x = 0;
	}

	if (y < 0) {
		addr -= y * pitch;
		y = 0;
	}

	state->pending_enable = enable;
	state->pending_pitch = pitch;
	state->pending_format = format;
	state->pending_addr = addr;
	state->pending_x = x;
	state->pending_y = y;
	state->pending_width = dest->x2 - dest->x1;
	state->pending_height = dest->y2 - dest->y1;
	state->pending_dirty = true;

	if (!enable)
		mtk_crtc_layer_off(mtk_plane->idx, state, plane->crtc);

	mtk_crtc_layer_config(mtk_plane->idx, state, plane->crtc);

	if (enable)
		mtk_crtc_layer_on(mtk_plane->idx, state, plane->crtc);
}

static struct drm_plane_state *mtk_plane_duplicate_state(struct drm_plane *plane)
{
	struct mtk_plane_state *old_state = to_mtk_plane_state(plane->state);
	struct mtk_plane_state *state;

	state = kzalloc(sizeof(*state), GFP_KERNEL);
	if (!state)
		return NULL;

	__drm_atomic_helper_plane_duplicate_state(plane, &state->base);

	WARN_ON(state->base.plane != plane);

	state->pending_config = old_state->pending_config;
	state->pending_enable = old_state->pending_enable;
	state->pending_addr = old_state->pending_addr;
	state->pending_pitch = old_state->pending_pitch;
	state->pending_format = old_state->pending_format;
	state->pending_x = old_state->pending_x;
	state->pending_y = old_state->pending_y;
	state->pending_width = old_state->pending_width;
	state->pending_height = old_state->pending_height;
	state->pending_dirty = old_state->pending_dirty;

	return &state->base;
}

static const struct drm_plane_funcs mtk_plane_funcs = {
	.update_plane = drm_atomic_helper_update_plane,
	.disable_plane = drm_atomic_helper_disable_plane,
	.destroy = drm_plane_cleanup,
	.reset = drm_atomic_helper_plane_reset,
	.atomic_duplicate_state = mtk_plane_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_plane_destroy_state,
};

static int mtk_plane_atomic_check(struct drm_plane *plane,
				  struct drm_plane_state *state)
{
	struct drm_framebuffer *fb = state->fb;
	struct drm_gem_object *gem;
	struct reservation_object *resv;
	struct drm_crtc_state *crtc_state;
	bool visible;
	int ret;
	struct drm_rect dest = {
		.x1 = state->crtc_x,
		.y1 = state->crtc_y,
		.x2 = state->crtc_x + state->crtc_w,
		.y2 = state->crtc_y + state->crtc_h,
	};
	struct drm_rect src = {
		/* 16.16 fixed point */
		.x1 = state->src_x,
		.y1 = state->src_y,
		.x2 = state->src_x + state->src_w,
		.y2 = state->src_y + state->src_h,
	};
	struct drm_rect clip = { 0, };

	if (!fb)
		return 0;

	if (!mtk_fb_get_gem_obj(fb)) {
		DRM_DEBUG_KMS("buffer is null\n");
		return -EFAULT;
	}

	if (!state->crtc)
		return 0;

	crtc_state = drm_atomic_get_crtc_state(state->state, state->crtc);
	if (IS_ERR(crtc_state))
		return PTR_ERR(crtc_state);

	clip.x2 = crtc_state->mode.hdisplay;
	clip.y2 = crtc_state->mode.vdisplay;

	ret = drm_plane_helper_check_update(plane, state->crtc, fb,
					    &src, &dest, &clip,
					    DRM_PLANE_HELPER_NO_SCALING,
					    DRM_PLANE_HELPER_NO_SCALING,
					    true, true, &visible);
	if (ret)
		return ret;

	/* Find pending fence from incoming FB, if any, and stash in state */
	gem = mtk_fb_get_gem_obj(fb);
	if (!gem->dma_buf || !gem->dma_buf->resv)
		return 0;

	resv = gem->dma_buf->resv;
	ww_mutex_lock(&resv->lock, NULL);
	state->fence = fence_get(reservation_object_get_excl(resv));
	ww_mutex_unlock(&resv->lock);

	return 0;
}

static void mtk_plane_atomic_update(struct drm_plane *plane,
				    struct drm_plane_state *old_state)
{
	struct mtk_plane_state *state = to_mtk_plane_state(plane->state);
	struct drm_crtc *crtc = state->base.crtc;
	struct drm_gem_object *gem;
	struct mtk_drm_gem_obj *mtk_gem;
	struct mtk_drm_plane *mtk_plane = to_mtk_plane(plane);
	struct drm_rect dest = {
		.x1 = state->base.crtc_x,
		.y1 = state->base.crtc_y,
		.x2 = state->base.crtc_x + state->base.crtc_w,
		.y2 = state->base.crtc_y + state->base.crtc_h,
	};
	struct drm_rect clip = { 0, };

	if (!crtc)
		return;

	clip.x2 = state->base.crtc->state->mode.hdisplay;
	clip.y2 = state->base.crtc->state->mode.vdisplay;
	drm_rect_intersect(&dest, &clip);

	gem = mtk_fb_get_gem_obj(state->base.fb);
	mtk_gem = to_mtk_gem_obj(gem);
	mtk_plane_config(mtk_plane, true, mtk_gem->dma_addr, &dest);

	mtk_drm_crtc_check_flush(crtc);
}

static void mtk_plane_atomic_disable(struct drm_plane *plane,
				     struct drm_plane_state *old_state)
{
	struct mtk_drm_plane *mtk_plane = to_mtk_plane(plane);
	struct drm_crtc *crtc = old_state->crtc;
	struct drm_rect dest = { 0, };

	if (!crtc)
		return;

	mtk_plane_config(mtk_plane, false, 0, &dest);

	mtk_drm_crtc_commit(crtc);
}

static const struct drm_plane_helper_funcs mtk_plane_helper_funcs = {
	.atomic_check = mtk_plane_atomic_check,
	.atomic_update = mtk_plane_atomic_update,
	.atomic_disable = mtk_plane_atomic_disable,
};

int mtk_plane_init(struct drm_device *dev, struct mtk_drm_plane *mtk_plane,
		   unsigned long possible_crtcs, enum drm_plane_type type,
		   unsigned int zpos)
{
	int err;

	err = drm_universal_plane_init(dev, &mtk_plane->base, possible_crtcs,
			&mtk_plane_funcs, formats, ARRAY_SIZE(formats), type);

	if (err) {
		DRM_ERROR("failed to initialize plane\n");
		return err;
	}

	drm_plane_helper_add(&mtk_plane->base, &mtk_plane_helper_funcs);
	mtk_plane->idx = zpos;

	return 0;
}

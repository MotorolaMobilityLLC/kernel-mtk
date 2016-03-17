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
#include <linux/dma-buf.h>
#include <linux/reservation.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_atomic_helper.h>

#include "mediatek_drm_plane.h"
#include "mediatek_drm_crtc.h"
#include "mediatek_drm_fb.h"
#include "mediatek_drm_gem.h"
#include "mediatek_drm_drv.h"


static const uint32_t formats[] = {
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_RGB565,
};

void mediatek_plane_finish_page_flip(struct mtk_drm_crtc *mtk_crtc)
{
	struct drm_device *dev = mtk_crtc->base.dev;

	if (mtk_crtc->fence)
		drm_fence_signal_and_put(&mtk_crtc->fence);
	mtk_crtc->fence = mtk_crtc->pending_fence;
	mtk_crtc->pending_fence = NULL;

	drm_send_vblank_event(dev, mtk_crtc->event->pipe, mtk_crtc->event);
	drm_crtc_vblank_put(&mtk_crtc->base);
	mtk_crtc->event = NULL;
}

static void mediatek_plane_update_cb(struct drm_reservation_cb *rcb,
	void *params)
{
	struct mtk_drm_crtc *mtk_crtc = params;
	struct drm_device *dev = mtk_crtc->base.dev;
	unsigned long flags;

	if (mtk_crtc->flip_obj && mtk_crtc->ops &&
		mtk_crtc->ops->plane_config)
		mtk_crtc->ops->plane_config(mtk_crtc, true,
			mtk_crtc->flip_obj->dma_addr);

	if (mtk_crtc->pending_commit && mtk_crtc->ops->crtc_commit) {
		mtk_crtc->ops->crtc_commit(mtk_crtc);
		mtk_crtc->pending_commit = false;
	}

	mtk_crtc->pending_config = false;

	spin_lock_irqsave(&dev->event_lock, flags);
	if (mtk_crtc->event) {
		mtk_crtc->pending_needs_vblank = true;
	} else {
		if (mtk_crtc->fence)
			drm_fence_signal_and_put(&mtk_crtc->fence);
		mtk_crtc->fence = mtk_crtc->pending_fence;
		mtk_crtc->pending_fence = NULL;
	}
	spin_unlock_irqrestore(&dev->event_lock, flags);
}

static int mediatek_plane_update_sync(struct mtk_drm_crtc *mtk_crtc,
	struct reservation_object *resv)
{
	struct fence *fence;
	int ret;

	ww_mutex_lock(&resv->lock, NULL);
	ret = reservation_object_reserve_shared(resv);
	if (ret < 0) {
		DRM_ERROR("Reserving space for shared fence failed: %d.\n",
			ret);
		goto err_mutex;
	}

	fence = drm_sw_fence_new(mtk_crtc->fence_context,
		atomic_add_return(1, &mtk_crtc->fence_seqno));
	if (IS_ERR(fence)) {
		ret = PTR_ERR(fence);
		DRM_ERROR("Failed to create fence: %d.\n", ret);
		goto err_mutex;
	}
	mtk_crtc->pending_config = true;
	mtk_crtc->pending_fence = fence;
	drm_reservation_cb_init(&mtk_crtc->rcb, mediatek_plane_update_cb,
		mtk_crtc);
	ret = drm_reservation_cb_add(&mtk_crtc->rcb, resv, false);
	if (ret < 0) {
		DRM_ERROR("Adding reservation to callback failed: %d.\n", ret);
		goto err_fence;
	}
	drm_reservation_cb_done(&mtk_crtc->rcb);
	reservation_object_add_shared_fence(resv, mtk_crtc->pending_fence);
	ww_mutex_unlock(&resv->lock);
	return 0;
err_fence:
	fence_put(mtk_crtc->pending_fence);
	mtk_crtc->pending_fence = NULL;
err_mutex:
	ww_mutex_unlock(&resv->lock);

	return ret;
}

static int mediatek_check_plane(struct drm_plane *plane,
	struct drm_framebuffer *fb)
{
	struct mediatek_drm_plane *mediatek_plane = to_mediatek_plane(plane);
	struct mtk_drm_fb *mtk_fb;
	struct mtk_drm_gem_obj *flip_obj;

	if (!fb)
		return 0;

	mtk_fb = to_mtk_fb(fb);
	flip_obj = to_mtk_gem_obj(mtk_fb->gem_obj[0]);

	if (!flip_obj) {
		DRM_DEBUG_KMS("buffer is null\n");
		return -EFAULT;
	}

	mediatek_plane->dma_addr = flip_obj->dma_addr;

	DRM_DEBUG_KMS("buffer: dma_addr = 0x%lx\n",
			(unsigned long)mediatek_plane->dma_addr);

	return 0;
}

static int
mediatek_update_plane(struct drm_plane *plane, struct drm_crtc *crtc,
		     struct drm_framebuffer *fb, int crtc_x, int crtc_y,
		     unsigned int crtc_w, unsigned int crtc_h,
		     uint32_t src_x, uint32_t src_y,
		     uint32_t src_w, uint32_t src_h)
{
	struct mtk_drm_crtc *mediatek_crtc = to_mtk_crtc(crtc);
	struct mtk_drm_fb *mtk_fb = to_mtk_fb(fb);

	mediatek_crtc->flip_obj = to_mtk_gem_obj(mtk_fb->gem_obj[0]);

	if (mtk_fb->gem_obj[0]->dma_buf && mtk_fb->gem_obj[0]->dma_buf->resv) {
		mediatek_plane_update_sync(mediatek_crtc,
			mtk_fb->gem_obj[0]->dma_buf->resv);
	} else {
		mediatek_plane_update_cb(NULL, mediatek_crtc);
	}

	return 0;
}

static struct drm_plane_funcs mediatek_plane_funcs = {
	.update_plane	= drm_atomic_helper_update_plane,
	.disable_plane	= drm_atomic_helper_disable_plane,
	.destroy	= drm_plane_cleanup,
	.reset = drm_atomic_helper_plane_reset,
	.atomic_duplicate_state = drm_atomic_helper_plane_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_plane_destroy_state,
};

static int mediatek_plane_atomic_check(struct drm_plane *plane,
	struct drm_plane_state *state)
{
	return mediatek_check_plane(plane, state->fb);
}

static void mediatek_plane_atomic_update(struct drm_plane *plane,
	struct drm_plane_state *old_state)
{
	struct drm_plane_state *state = plane->state;

	if (!state->crtc)
		return;

	mediatek_update_plane(plane, state->crtc, state->fb,
		state->crtc_x, state->crtc_y,
		state->crtc_w, state->crtc_h,
		state->src_x, state->src_y,
		state->src_w, state->src_h);
}

static void mediatek_plane_atomic_disable(struct drm_plane *plane,
	struct drm_plane_state *old_state)
{
	struct mtk_drm_crtc *mediatek_crtc = to_mtk_crtc(old_state->crtc);

	if (!old_state->crtc)
		return;

	if (mediatek_crtc && mediatek_crtc->ops->plane_config)
		mediatek_crtc->ops->plane_config(mediatek_crtc,
			false, 0);
}

static const struct drm_plane_helper_funcs plane_helper_funcs = {
	.atomic_check = mediatek_plane_atomic_check,
	.atomic_update = mediatek_plane_atomic_update,
	.atomic_disable = mediatek_plane_atomic_disable,
};

static void mediatek_plane_attach_zpos_property(struct drm_plane *plane,
	unsigned int zpos, unsigned int max_plane)
{
	struct drm_device *dev = plane->dev;
	struct mtk_drm_private *dev_priv = dev->dev_private;
	struct drm_property *prop;

	prop = dev_priv->plane_zpos_property;
	if (!prop) {
		prop = drm_property_create_range(dev, DRM_MODE_PROP_IMMUTABLE,
						 "zpos", 0, max_plane - 1);
		if (!prop)
			return;

		dev_priv->plane_zpos_property = prop;
	}

	drm_object_attach_property(&plane->base, prop, zpos);
}

int mediatek_plane_init(struct drm_device *dev,
		      struct mediatek_drm_plane *mediatek_plane,
		      unsigned long possible_crtcs, enum drm_plane_type type,
		      unsigned int zpos, unsigned int max_plane)
{
	int err;

	err = drm_universal_plane_init(dev, &mediatek_plane->base,
		possible_crtcs, &mediatek_plane_funcs, formats,
		ARRAY_SIZE(formats), type);

	if (err) {
		DRM_ERROR("failed to initialize plane\n");
		return err;
	}

	drm_plane_helper_add(&mediatek_plane->base, &plane_helper_funcs);

	mediatek_plane->zpos = zpos;

	if (type == DRM_PLANE_TYPE_OVERLAY)
		mediatek_plane_attach_zpos_property(&mediatek_plane->base,
			zpos, max_plane);

	return 0;
}

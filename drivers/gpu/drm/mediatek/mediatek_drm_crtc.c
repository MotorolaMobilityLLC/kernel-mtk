/*
 * Copyright (c) 2015 MediaTek Inc.
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
#include <drm/drm_gem.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <linux/dma-buf.h>
#include <linux/reservation.h>

#include "mediatek_drm_drv.h"
#include "mediatek_drm_crtc.h"
#include "mediatek_drm_fb.h"
#include "mediatek_drm_gem.h"
#include "mediatek_drm_plane.h"


static void mediatek_drm_crtc_pending_ovl_cursor_config(
		struct mtk_drm_crtc *mtk_crtc, bool enable, dma_addr_t addr)
{
	if (mtk_crtc->ops && mtk_crtc->ops->ovl_layer_config_cursor)
		mtk_crtc->ops->ovl_layer_config_cursor(mtk_crtc, enable, addr);
}

static int mtk_drm_crtc_cursor_set(struct drm_crtc *crtc,
	struct drm_file *file_priv,	uint32_t handle,
	uint32_t width,	uint32_t height)
{
	struct drm_device *dev = crtc->dev;
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(crtc);
	struct drm_gem_object *obj;
	struct mtk_drm_gem_obj *mtk_gem;

	if (!handle) {
		/* turn off cursor */
		obj = NULL;
		mtk_gem = NULL;
		goto finish;
	}

	if ((width != 64) || (height != 64)) {
		DRM_ERROR("bad cursor width or height %dx%d\n", width, height);
		return -EINVAL;
	}

	obj = drm_gem_object_lookup(dev, file_priv, handle);
	if (!obj) {
		DRM_ERROR("not find cursor obj %x crtc %p\n", handle, mtk_crtc);
		return -ENOENT;
	}

	mtk_gem = to_mtk_gem_obj(obj);

finish:
	if (mtk_crtc->cursor_obj)
		drm_gem_object_unreference_unlocked(mtk_crtc->cursor_obj);

	mtk_crtc->cursor_w = width;
	mtk_crtc->cursor_h = height;
	mtk_crtc->cursor_obj = obj;

	if (mtk_gem) {
		mediatek_drm_crtc_pending_ovl_cursor_config(mtk_crtc,
			true, mtk_gem->dma_addr);
	} else {
		mediatek_drm_crtc_pending_ovl_cursor_config(mtk_crtc,
			false, 0);
	}

	return 0;
}

static int mtk_drm_crtc_cursor_move(struct drm_crtc *crtc, int x, int y)
{
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(crtc);
	struct mtk_drm_gem_obj *mtk_gem;

	if (!mtk_crtc->cursor_obj) {
		/* DRM_ERROR("mtk_drm_crtc_cursor_move no obj [%p]\n", crtc); */
		return 0;
	}

	mtk_gem = to_mtk_gem_obj(mtk_crtc->cursor_obj);

	if (x < 0)
		x = 0;

	if (y < 0)
		y = 0;

	mtk_crtc->cursor_x = x;
	mtk_crtc->cursor_y = y;

	mediatek_drm_crtc_pending_ovl_cursor_config(mtk_crtc,
		true, mtk_gem->dma_addr);

	return 0;
}

static void mtk_drm_crtc_destroy(struct drm_crtc *crtc)
{
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(crtc);

	struct drm_device *dev = crtc->dev;
	unsigned long flags;

	spin_lock_irqsave(&dev->event_lock, flags);
	if (mtk_crtc->fence)
		drm_fence_signal_and_put(&mtk_crtc->fence);

	if (mtk_crtc->pending_fence)
		drm_fence_signal_and_put(&mtk_crtc->pending_fence);
	spin_unlock_irqrestore(&dev->event_lock, flags);

	drm_crtc_cleanup(crtc);
}

static void mtk_drm_crtc_dpms(struct drm_crtc *crtc, int mode)
{
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(crtc);

	DRM_INFO("mtk_drm_crtc_dpms %d => %d\n", mtk_crtc->dpms, mode);
	if (mtk_crtc->dpms == mode)
		return;

	switch (mode) {
	case DRM_MODE_DPMS_ON:
		mtk_crtc->ops->dpms(mtk_crtc, mode);
		drm_crtc_vblank_on(crtc);
		break;
	case DRM_MODE_DPMS_STANDBY:
	case DRM_MODE_DPMS_SUSPEND:
	case DRM_MODE_DPMS_OFF:
		drm_crtc_vblank_off(crtc);
		mtk_crtc->ops->dpms(mtk_crtc, mode);
		break;
	default:
		DRM_DEBUG_KMS("unspecified mode %d\n", mode);
		break;
	}

	mtk_crtc->dpms = mode;
}

static void mtk_drm_crtc_prepare(struct drm_crtc *crtc)
{
	/* drm framework doesn't check NULL. */
}

static void mtk_drm_crtc_commit(struct drm_crtc *crtc)
{
	/*
	 * when set_crtc is requested from user or at booting time,
	 * crtc->commit would be called without dpms call so if dpms is
	 * no power on then crtc->dpms should be called
	 * with DRM_MODE_DPMS_ON for the hardware power to be on.
	 */
}

static bool mtk_drm_crtc_mode_fixup(struct drm_crtc *crtc,
		const struct drm_display_mode *mode,
		struct drm_display_mode *adjusted_mode)
{
	/* drm framework doesn't check NULL */
	return true;
}

static void mtk_drm_crtc_mode_set_nofb(struct drm_crtc *crtc)
{
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(crtc);

	if (WARN_ON(!crtc->state))
		return;

	if (mtk_crtc->ops && mtk_crtc->ops->crtc_config)
		mtk_crtc->ops->crtc_config(mtk_crtc,
			crtc->mode.hdisplay, crtc->mode.vdisplay);
}

int mtk_drm_crtc_enable_vblank(struct drm_device *drm, int pipe)
{
	struct mtk_drm_private *priv =
		(struct mtk_drm_private *)drm->dev_private;
	struct mtk_drm_crtc *mtk_crtc;

	if (pipe >= MAX_CRTC || pipe < 0) {
		DRM_ERROR(" - %s: invalid crtc (%d)\n", __func__, pipe);
		return -EINVAL;
	}

	mtk_crtc = to_mtk_crtc(priv->crtc[pipe]);

	if (mtk_crtc->ops->enable_vblank)
		mtk_crtc->ops->enable_vblank(mtk_crtc);

	return 0;
}

void mtk_drm_crtc_disable_vblank(struct drm_device *drm, int pipe)
{
	struct mtk_drm_private *priv =
		(struct mtk_drm_private *)drm->dev_private;
	struct mtk_drm_crtc *mtk_crtc;

	if (pipe >= MAX_CRTC || pipe < 0) {
		DRM_ERROR(" - %s: invalid crtc (%d)\n", __func__, pipe);
		return;
	}

	mtk_crtc = to_mtk_crtc(priv->crtc[pipe]);
	if (mtk_crtc->ops->disable_vblank)
		mtk_crtc->ops->disable_vblank(mtk_crtc);
}

static void mtk_drm_crtc_disable(struct drm_crtc *crtc)
{
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(crtc);

	DRM_INFO("mtk_drm_crtc_disable %d\n", crtc->base.id);

	if (mtk_crtc->ops && mtk_crtc->ops->plane_config)
		mtk_crtc->ops->plane_config(mtk_crtc, false, 0);
}

static void mtk_crtc_atomic_begin(struct drm_crtc *crtc)
{
	struct drm_crtc_state *state = crtc->state;
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(crtc);

	if (state->event) {
		state->event->pipe = drm_crtc_index(crtc);
		WARN_ON(drm_crtc_vblank_get(crtc) != 0);
		mtk_crtc->event = state->event;
		state->event = NULL;
	}
}

static void mtk_crtc_atomic_flush(struct drm_crtc *crtc)
{
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(crtc);

	if (mtk_crtc->pending_config)
		mtk_crtc->pending_commit = true;
	else if (mtk_crtc->ops->crtc_commit)
		mtk_crtc->ops->crtc_commit(mtk_crtc);
}

static const struct drm_crtc_funcs mediatek_crtc_funcs = {
	.cursor_set		= mtk_drm_crtc_cursor_set,
	.cursor_move	= mtk_drm_crtc_cursor_move,
	.set_config		= drm_atomic_helper_set_config,
	.page_flip		= drm_atomic_helper_page_flip,
	.destroy		= mtk_drm_crtc_destroy,
	.reset			= drm_atomic_helper_crtc_reset,
	.atomic_duplicate_state = drm_atomic_helper_crtc_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_crtc_destroy_state,
};

static struct drm_crtc_helper_funcs mediatek_crtc_helper_funcs = {
	.dpms		= mtk_drm_crtc_dpms,
	.prepare	= mtk_drm_crtc_prepare,
	.commit		= mtk_drm_crtc_commit,
	.mode_fixup	= mtk_drm_crtc_mode_fixup,
	.mode_set_nofb  = mtk_drm_crtc_mode_set_nofb,
	.disable	= mtk_drm_crtc_disable,
	.atomic_begin	= mtk_crtc_atomic_begin,
	.atomic_flush	= mtk_crtc_atomic_flush,
};

struct mtk_drm_crtc *mtk_drm_crtc_create(
	struct drm_device *drm_dev, struct drm_plane *primary, int pipe,
	const struct mediatek_drm_crtc_ops *ops,
	void *ctx)
{
	struct mtk_drm_private *priv = drm_dev->dev_private;
	struct mtk_drm_crtc *mtk_crtc;
	int ret;

	mtk_crtc = kzalloc(sizeof(*mtk_crtc), GFP_KERNEL);
	if (!mtk_crtc) {
		DRM_ERROR("failed to allocate mtk crtc\n");
		return ERR_PTR(-ENOMEM);
	}

	mtk_crtc->pipe = pipe;
	mtk_crtc->ops = ops;
	mtk_crtc->ctx = ctx;

	priv->crtc[pipe] = &mtk_crtc->base;

	ret = drm_crtc_init_with_planes(drm_dev, &mtk_crtc->base, primary, NULL,
		&mediatek_crtc_funcs);
	if (ret)
		goto err;

	drm_crtc_helper_add(&mtk_crtc->base, &mediatek_crtc_helper_funcs);

	mtk_crtc->fence_context = fence_context_alloc(1);
	atomic_set(&mtk_crtc->fence_seqno, 0);

	return mtk_crtc;
err:
	kfree(mtk_crtc);
	return ERR_PTR(ret);
}


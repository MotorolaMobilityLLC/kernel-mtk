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

#include <asm/barrier.h>
#include <drm/drmP.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_plane_helper.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <soc/mediatek/smi.h>

#include "mtk_drm_drv.h"
#include "mtk_drm_crtc.h"
#include "mtk_drm_ddp.h"
#include "mtk_drm_ddp_comp.h"
#include "mtk_drm_gem.h"
#include "mtk_drm_plane.h"

/**
 * struct mtk_drm_crtc - MediaTek specific crtc structure.
 * @base: crtc object.
 * @pipe: a crtc index created at load() with a new crtc object creation
 *	and the crtc object would be set to private->crtc array
 *	to get a crtc object corresponding to this pipe from private->crtc
 *	array when irq interrupt occurred. the reason of using this pipe is that
 *	drm framework doesn't support multiple irq yet.
 *	we can refer to the crtc to current hardware interrupt occurred through
 *	this pipe value.
 * @enabled: records whether crtc_enable succeeded
 * @do_flush: enabled by atomic_flush, causes plane atomic_update to commit
 *            changed state immediately.
 * @planes: array of 4 mtk_drm_plane structures, one for each overlay plane
 * @config_regs: memory mapped mmsys configuration register space
 * @mutex: handle to one of the ten disp_mutex streams
 * @ddp_comp_nr: number of components in ddp_comp
 * @ddp_comp: array of pointers the mtk_ddp_comp structures used by this crtc
 */
struct mtk_drm_crtc {
	struct drm_crtc			base;
	unsigned int			pipe;
	bool				enabled;

	bool				do_flush;

	struct mtk_drm_plane		planes[OVL_LAYER_NR];

	void __iomem			*config_regs;
	struct mtk_disp_mutex		*mutex;
	unsigned int			ddp_comp_nr;
	struct mtk_ddp_comp		**ddp_comp;
};

struct mtk_crtc_state {
	struct drm_crtc_state		base;
	struct drm_pending_vblank_event	*event;

	bool				pending_needs_vblank;

	bool				pending_config;
	unsigned int			pending_width;
	unsigned int			pending_height;
	unsigned int			pending_vrefresh;
};

static inline struct mtk_drm_crtc *to_mtk_crtc(struct drm_crtc *c)
{
	return container_of(c, struct mtk_drm_crtc, base);
}

static inline struct mtk_crtc_state *to_mtk_crtc_state(struct drm_crtc_state *s)
{
	return container_of(s, struct mtk_crtc_state, base);
}

static struct mtk_drm_crtc *mtk_crtc_by_comp(struct mtk_drm_private *priv,
					     struct mtk_ddp_comp *ddp_comp)
{
	struct mtk_drm_crtc *mtk_crtc;
	int i;

	for (i = 0; i < MAX_CRTC; i++) {
		mtk_crtc = to_mtk_crtc(priv->crtc[i]);
		if (mtk_crtc->ddp_comp[0] == ddp_comp)
			return mtk_crtc;
	}

	return NULL;
}

static void mtk_drm_crtc_finish_page_flip(struct mtk_drm_crtc *mtk_crtc)
{
	struct drm_crtc *crtc = &mtk_crtc->base;
	struct mtk_crtc_state *state = to_mtk_crtc_state(crtc->state);
	unsigned long flags;

	spin_lock_irqsave(&crtc->dev->event_lock, flags);
	drm_send_vblank_event(crtc->dev, state->event->pipe, state->event);
	drm_crtc_vblank_put(crtc);
	state->event = NULL;
	spin_unlock_irqrestore(&crtc->dev->event_lock, flags);
}

static void mtk_drm_finish_page_flip(struct mtk_drm_crtc *mtk_crtc)
{
	struct mtk_crtc_state *state = to_mtk_crtc_state(mtk_crtc->base.state);

	drm_handle_vblank(mtk_crtc->base.dev, mtk_crtc->pipe);
	if (state->pending_needs_vblank) {
		mtk_drm_crtc_finish_page_flip(mtk_crtc);
		state->pending_needs_vblank = false;
	}
}

static void mtk_drm_crtc_destroy(struct drm_crtc *crtc)
{
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(crtc);
	int i;

	for (i = 0; i < mtk_crtc->ddp_comp_nr; i++)
		clk_unprepare(mtk_crtc->ddp_comp[i]->clk);

	mtk_disp_mutex_put(mtk_crtc->mutex);

	drm_crtc_cleanup(crtc);
}

static void mtk_drm_crtc_reset(struct drm_crtc *crtc)
{
	struct mtk_crtc_state *state;

	if (crtc->state && crtc->state->mode_blob)
		drm_property_unreference_blob(crtc->state->mode_blob);

	kfree(crtc->state);
	state = kzalloc(sizeof(*state), GFP_KERNEL);
	crtc->state = &state->base;

	if (state)
		crtc->state->crtc = crtc;
}

static struct drm_crtc_state *mtk_drm_crtc_duplicate_state(struct drm_crtc *crtc)
{
	struct mtk_crtc_state *state;

	state = kzalloc(sizeof(*state), GFP_KERNEL);
	if (!state)
		return NULL;

	__drm_atomic_helper_crtc_duplicate_state(crtc, &state->base);

	WARN_ON(state->base.crtc != crtc);
	state->base.crtc = crtc;

	return &state->base;
}

static bool mtk_drm_crtc_mode_fixup(struct drm_crtc *crtc,
		const struct drm_display_mode *mode,
		struct drm_display_mode *adjusted_mode)
{
	/* Nothing to do here, but this callback is mandatory. */
	return true;
}

static void mtk_drm_crtc_mode_set_nofb(struct drm_crtc *crtc)
{
	struct mtk_crtc_state *state = to_mtk_crtc_state(crtc->state);
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(crtc);
	struct mtk_ddp_comp *ovl = mtk_crtc->ddp_comp[0];

	state->pending_width = crtc->mode.hdisplay;
	state->pending_height = crtc->mode.vdisplay;
	state->pending_vrefresh = crtc->mode.vrefresh;
	wmb();
	state->pending_config = true;

	if (ovl->do_shadow_reg)
		mtk_ddp_comp_config(ovl, state->pending_width,
				    state->pending_height,
				    state->pending_vrefresh);
}

int mtk_drm_crtc_enable_vblank(struct drm_device *drm, unsigned int pipe)
{
	struct mtk_drm_private *priv = drm->dev_private;
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(priv->crtc[pipe]);
	struct mtk_ddp_comp *ovl = mtk_crtc->ddp_comp[0];

	mtk_ddp_comp_enable_vblank(ovl);

	return 0;
}

void mtk_drm_crtc_disable_vblank(struct drm_device *drm, unsigned int pipe)
{
	struct mtk_drm_private *priv = drm->dev_private;
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(priv->crtc[pipe]);
	struct mtk_ddp_comp *ovl = mtk_crtc->ddp_comp[0];

	mtk_ddp_comp_disable_vblank(ovl);
}

static int mtk_crtc_ddp_power_on(struct mtk_drm_crtc *mtk_crtc)
{
	int ret;
	int i;

	DRM_DEBUG_DRIVER("%s\n", __func__);
	for (i = 0; i < mtk_crtc->ddp_comp_nr; i++) {
		ret = clk_enable(mtk_crtc->ddp_comp[i]->clk);
		if (ret) {
			DRM_ERROR("Failed to enable clock %d: %d\n", i, ret);
			goto err;
		}
	}

	return 0;
err:
	while (--i >= 0)
		clk_disable(mtk_crtc->ddp_comp[i]->clk);
	return ret;
}

static void mtk_crtc_ddp_power_off(struct mtk_drm_crtc *mtk_crtc)
{
	int i;

	DRM_DEBUG_DRIVER("%s\n", __func__);
	for (i = 0; i < mtk_crtc->ddp_comp_nr; i++)
		clk_disable(mtk_crtc->ddp_comp[i]->clk);
}

static int mtk_crtc_ddp_hw_init(struct mtk_drm_crtc *mtk_crtc)
{
	struct drm_crtc *crtc = &mtk_crtc->base;
	unsigned int width, height, vrefresh;
	int ret;
	int i;

	DRM_DEBUG_DRIVER("%s\n", __func__);
	if (WARN_ON(!crtc->state))
		return -EINVAL;

	width = crtc->state->adjusted_mode.hdisplay;
	height = crtc->state->adjusted_mode.vdisplay;
	vrefresh = crtc->state->adjusted_mode.vrefresh;

	ret = pm_runtime_get_sync(crtc->dev->dev);
	if (ret < 0) {
		DRM_ERROR("Failed to enable power domain: %d\n", ret);
		return ret;
	}

	ret = mtk_disp_mutex_prepare(mtk_crtc->mutex);
	if (ret < 0) {
		DRM_ERROR("Failed to enable mutex clock: %d\n", ret);
		goto err_pm_runtime_put;
	}

	ret = mtk_crtc_ddp_power_on(mtk_crtc);
	if (ret < 0) {
		DRM_ERROR("Failed to enable component clocks: %d\n", ret);
		goto err_mutex_unprepare;
	}

	DRM_DEBUG_DRIVER("mediatek_ddp_ddp_path_setup\n");
	for (i = 0; i < mtk_crtc->ddp_comp_nr - 1; i++) {
		mtk_ddp_add_comp_to_path(mtk_crtc->mutex,
					 mtk_crtc->config_regs,
					 mtk_crtc->ddp_comp[i]->id,
					 mtk_crtc->ddp_comp[i + 1]->id);
		mtk_disp_mutex_add_comp(mtk_crtc->mutex,
					mtk_crtc->ddp_comp[i]->id);
	}
	mtk_disp_mutex_add_comp(mtk_crtc->mutex, mtk_crtc->ddp_comp[i]->id);
	mtk_disp_mutex_enable(mtk_crtc->mutex);

	DRM_DEBUG_DRIVER("ddp_disp_path_power_on %dx%d\n", width, height);
	for (i = 0; i < mtk_crtc->ddp_comp_nr; i++) {
		struct mtk_ddp_comp *comp = mtk_crtc->ddp_comp[i];

		mtk_ddp_comp_config(comp, width, height, vrefresh);
		mtk_ddp_comp_start(comp);
	}

	return 0;

err_mutex_unprepare:
	mtk_disp_mutex_unprepare(mtk_crtc->mutex);
err_pm_runtime_put:
	pm_runtime_put(crtc->dev->dev);
	return ret;
}

static void mtk_crtc_ddp_hw_fini(struct mtk_drm_crtc *mtk_crtc)
{
	struct drm_device *drm = mtk_crtc->base.dev;
	int i;

	DRM_DEBUG_DRIVER("%s\n", __func__);
	for (i = 0; i < mtk_crtc->ddp_comp_nr; i++)
		mtk_ddp_comp_stop(mtk_crtc->ddp_comp[i]);
	for (i = 0; i < mtk_crtc->ddp_comp_nr; i++)
		mtk_disp_mutex_remove_comp(mtk_crtc->mutex,
					   mtk_crtc->ddp_comp[i]->id);
	mtk_disp_mutex_disable(mtk_crtc->mutex);
	for (i = 0; i < mtk_crtc->ddp_comp_nr - 1; i++) {
		mtk_ddp_remove_comp_from_path(mtk_crtc->mutex,
					      mtk_crtc->config_regs,
					      mtk_crtc->ddp_comp[i]->id,
					      mtk_crtc->ddp_comp[i + 1]->id);
		mtk_disp_mutex_remove_comp(mtk_crtc->mutex,
					   mtk_crtc->ddp_comp[i]->id);
	}
	mtk_disp_mutex_remove_comp(mtk_crtc->mutex, mtk_crtc->ddp_comp[i]->id);
	mtk_crtc_ddp_power_off(mtk_crtc);
	mtk_disp_mutex_unprepare(mtk_crtc->mutex);

	pm_runtime_put(drm->dev);
}

void mtk_crtc_plane_config(struct mtk_drm_plane *plane,
			   struct mtk_plane_state *state)
{
	struct mtk_drm_crtc *mtk_crtc;
	struct mtk_ddp_comp *ovl;

	if (state->base.crtc)
		mtk_crtc = to_mtk_crtc(state->base.crtc);
	else if (plane->base.crtc)
		mtk_crtc = to_mtk_crtc(plane->base.crtc);
	else {
		DRM_ERROR("plane config w/o crtc\n");
		return;
	}

	ovl = mtk_crtc->ddp_comp[0];

	if (ovl->do_shadow_reg && !state->pending.enable)
		mtk_ddp_comp_layer_off(ovl, plane->idx);

	if (ovl->do_shadow_reg)
		mtk_ddp_comp_layer_config(ovl, plane->idx, state);

	if (ovl->do_shadow_reg && state->pending.enable)
		mtk_ddp_comp_layer_on(ovl, plane->idx);
}

static void mtk_drm_crtc_enable(struct drm_crtc *crtc)
{
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(crtc);
	struct mtk_ddp_comp *ovl = mtk_crtc->ddp_comp[0];
	int ret;

	DRM_DEBUG_DRIVER("%s %d\n", __func__, crtc->base.id);

	ret = mtk_smi_larb_get(ovl->larb_dev);
	if (ret) {
		DRM_ERROR("Failed to get larb: %d\n", ret);
		return;
	}

	ret = mtk_crtc_ddp_hw_init(mtk_crtc);
	if (ret) {
		mtk_smi_larb_put(ovl->larb_dev);
		return;
	}

	drm_crtc_vblank_on(crtc);
	mtk_crtc->enabled = true;
}

static void mtk_drm_crtc_disable(struct drm_crtc *crtc)
{
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(crtc);
	struct mtk_ddp_comp *ovl = mtk_crtc->ddp_comp[0];
	int i;

	DRM_DEBUG_DRIVER("%s %d\n", __func__, crtc->base.id);
	if (WARN_ON(!mtk_crtc->enabled))
		return;

	/* Set all pending plane state to disabled */
	for (i = 0; i < OVL_LAYER_NR; i++) {
		struct drm_plane *plane = &mtk_crtc->planes[i].base;
		struct mtk_plane_state *plane_state;

		plane_state = to_mtk_plane_state(plane->state);
		plane_state->pending.enable = false;
		plane_state->pending.config = true;
	}

	/* Wait for planes to be disabled */
	drm_crtc_wait_one_vblank(crtc);

	drm_crtc_vblank_off(crtc);
	mtk_crtc_ddp_hw_fini(mtk_crtc);
	mtk_smi_larb_put(ovl->larb_dev);

	mtk_crtc->do_flush = false;
	mtk_crtc->enabled = false;
}

static void mtk_drm_crtc_atomic_begin(struct drm_crtc *crtc,
				      struct drm_crtc_state *old_crtc_state)
{
	struct mtk_crtc_state *state = to_mtk_crtc_state(crtc->state);

	if (state->base.event) {
		state->base.event->pipe = drm_crtc_index(crtc);
		WARN_ON(drm_crtc_vblank_get(crtc) != 0);
		state->event = state->base.event;
		state->base.event = NULL;
	}
}

void mtk_drm_crtc_commit(struct drm_crtc *crtc)
{
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(crtc);
	struct mtk_ddp_comp *ovl = mtk_crtc->ddp_comp[0];
	unsigned int i;

	for (i = 0; i < OVL_LAYER_NR; i++) {
		struct drm_plane *plane = &mtk_crtc->planes[i].base;
		struct mtk_plane_state *plane_state;

		plane_state = to_mtk_plane_state(plane->state);
		if (plane_state->pending.dirty) {
			plane_state->pending.config = true;
			plane_state->pending.dirty = false;
		}
	}

	if (ovl->do_shadow_reg) {
		mtk_ddp_get_mutex(mtk_crtc->mutex);
		mtk_ddp_release_mutex(mtk_crtc->mutex);
	}
}

void mtk_drm_crtc_check_flush(struct drm_crtc *crtc)
{
	struct mtk_crtc_state *state = to_mtk_crtc_state(crtc->state);
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(crtc);

	if (mtk_crtc->do_flush) {
		if (state->event)
			state->pending_needs_vblank = true;
		mtk_drm_crtc_commit(crtc);
		mtk_crtc->do_flush = false;
	}
}

static void mtk_drm_crtc_atomic_flush(struct drm_crtc *crtc,
				      struct drm_crtc_state *old_crtc_state)
{
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(crtc);

	mtk_crtc->do_flush = true;
	mtk_drm_crtc_check_flush(crtc);
}

static const struct drm_crtc_funcs mtk_crtc_funcs = {
	.set_config		= drm_atomic_helper_set_config,
	.page_flip		= drm_atomic_helper_page_flip,
	.destroy		= mtk_drm_crtc_destroy,
	.reset			= mtk_drm_crtc_reset,
	.atomic_duplicate_state	= mtk_drm_crtc_duplicate_state,
	.atomic_destroy_state	= drm_atomic_helper_crtc_destroy_state,
};

static const struct drm_crtc_helper_funcs mtk_crtc_helper_funcs = {
	.mode_fixup	= mtk_drm_crtc_mode_fixup,
	.mode_set_nofb	= mtk_drm_crtc_mode_set_nofb,
	.enable		= mtk_drm_crtc_enable,
	.disable	= mtk_drm_crtc_disable,
	.atomic_begin	= mtk_drm_crtc_atomic_begin,
	.atomic_flush	= mtk_drm_crtc_atomic_flush,
};

static int mtk_drm_crtc_init(struct drm_device *drm,
		struct mtk_drm_crtc *mtk_crtc, struct drm_plane *primary,
		struct drm_plane *cursor, unsigned int pipe)
{
	int ret;

	ret = drm_crtc_init_with_planes(drm, &mtk_crtc->base, primary, cursor,
			&mtk_crtc_funcs);
	if (ret)
		goto err_cleanup_crtc;

	drm_crtc_helper_add(&mtk_crtc->base, &mtk_crtc_helper_funcs);

	mtk_crtc->pipe = pipe;

	return 0;

err_cleanup_crtc:
	drm_crtc_cleanup(&mtk_crtc->base);
	return ret;
}

void mtk_crtc_ddp_irq(struct drm_device *drm_dev, struct mtk_ddp_comp *ovl)
{
	struct mtk_drm_private *priv = drm_dev->dev_private;
	struct mtk_drm_crtc *mtk_crtc;
	struct mtk_crtc_state *state;
	unsigned int i;

	mtk_crtc = mtk_crtc_by_comp(priv, ovl);
	if (WARN_ON(!mtk_crtc))
		return;

	state = to_mtk_crtc_state(mtk_crtc->base.state);

	/*
	 * TODO: instead of updating the registers here, we should prepare
	 * working registers in atomic_commit and let the hardware command
	 * queue update module registers on vblank.
	 */
	if (!ovl->do_shadow_reg) {
		if (state->pending_config) {
			mtk_ddp_comp_config(ovl, state->pending_width,
					    state->pending_height,
					    state->pending_vrefresh);

			state->pending_config = false;
		}

		for (i = 0; i < OVL_LAYER_NR; i++) {
			struct drm_plane *plane = &mtk_crtc->planes[i].base;
			struct mtk_plane_state *plane_state;

			plane_state = to_mtk_plane_state(plane->state);

			if (plane_state->pending.config) {
				if (!plane_state->pending.enable)
					mtk_ddp_comp_layer_off(ovl, i);

				mtk_ddp_comp_layer_config(ovl, i, plane_state);

				if (plane_state->pending.enable)
					mtk_ddp_comp_layer_on(ovl, i);

				plane_state->pending.config = false;
			}
		}
	}

	mtk_drm_finish_page_flip(mtk_crtc);
}

int mtk_drm_crtc_create(struct drm_device *drm_dev,
			const enum mtk_ddp_comp_id *path, unsigned int path_len)
{
	struct mtk_drm_private *priv = drm_dev->dev_private;
	struct device *dev = drm_dev->dev;
	struct mtk_drm_crtc *mtk_crtc;
	enum drm_plane_type type;
	unsigned int zpos;
	int pipe = priv->num_pipes;
	int ret;
	int i;

	mtk_crtc = devm_kzalloc(dev, sizeof(*mtk_crtc), GFP_KERNEL);
	if (!mtk_crtc)
		return -ENOMEM;

	mtk_crtc->config_regs = priv->config_regs;
	mtk_crtc->ddp_comp_nr = path_len;
	mtk_crtc->ddp_comp = devm_kmalloc_array(dev, mtk_crtc->ddp_comp_nr,
						sizeof(*mtk_crtc->ddp_comp),
						GFP_KERNEL);

	mtk_crtc->mutex = mtk_disp_mutex_get(priv->mutex_dev, pipe);
	if (IS_ERR(mtk_crtc->mutex)) {
		ret = PTR_ERR(mtk_crtc->mutex);
		dev_err(dev, "Failed to get mutex: %d\n", ret);
		return ret;
	}

	for (i = 0; i < mtk_crtc->ddp_comp_nr; i++) {
		enum mtk_ddp_comp_id comp_id = path[i];
		struct mtk_ddp_comp *comp;

		comp = priv->ddp_comp[comp_id];
		if (!comp) {
			dev_err(dev, "Component %s not initialized\n",
				priv->comp_node[comp_id]->full_name);
			ret = -ENODEV;
			goto unprepare;
		}

		ret = clk_prepare(comp->clk);
		if (ret) {
			dev_err(dev,
				"Failed to prepare clock for component %s: %d\n",
				priv->comp_node[comp_id]->full_name, ret);
			goto unprepare;
		}

		mtk_crtc->ddp_comp[i] = comp;
	}

	for (zpos = 0; zpos < OVL_LAYER_NR; zpos++) {
		type = (zpos == 0) ? DRM_PLANE_TYPE_PRIMARY :
				(zpos == 1) ? DRM_PLANE_TYPE_CURSOR :
						DRM_PLANE_TYPE_OVERLAY;
		ret = mtk_plane_init(drm_dev, &mtk_crtc->planes[zpos],
				     BIT(pipe), type, zpos);
		if (ret)
			goto unprepare;
	}

	ret = mtk_drm_crtc_init(drm_dev, mtk_crtc, &mtk_crtc->planes[0].base,
				&mtk_crtc->planes[1].base, pipe);
	if (ret < 0)
		goto unprepare;

	priv->crtc[pipe] = &mtk_crtc->base;
	priv->num_pipes++;

	return 0;

unprepare:
	while (--i >= 0)
		clk_unprepare(mtk_crtc->ddp_comp[i]->clk);

	return ret;
}

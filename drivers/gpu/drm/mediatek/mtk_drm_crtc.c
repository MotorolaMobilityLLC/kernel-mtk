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
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_plane_helper.h>
#include <linux/clk.h>
#include <linux/component.h>
#include <linux/dma-buf.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/reservation.h>
#include <soc/mediatek/smi.h>

#include "mtk_drm_drv.h"
#include "mtk_drm_crtc.h"
#include "mtk_drm_ddp.h"
#include "mtk_drm_ddp_comp.h"
#include "mtk_drm_gem.h"
#include "mtk_drm_plane.h"

struct mtk_crtc_ddp_context;

/*
 * MediaTek specific crtc structure.
 *
 * @base: crtc object.
 * @pipe: a crtc index created at load() with a new crtc object creation
 *	and the crtc object would be set to private->crtc array
 *	to get a crtc object corresponding to this pipe from private->crtc
 *	array when irq interrupt occurred. the reason of using this pipe is that
 *	drm framework doesn't support multiple irq yet.
 *	we can refer to the crtc to current hardware interrupt occurred through
 *	this pipe value.
 */
struct mtk_drm_crtc {
	struct drm_crtc			base;
	unsigned int			pipe;

	bool				do_flush;
	bool				do_shadow_reg;

	struct mtk_drm_plane		planes[OVL_LAYER_NR];

	void __iomem			*config_regs;
	struct mtk_disp_mutex		*mutex;
	u32				ddp_comp_nr;
	struct mtk_ddp_comp		**ddp_comp;
};

struct mtk_disp_ovl {
	struct mtk_ddp_comp		ddp_comp;
	struct mtk_drm_crtc		*crtc;
};

static inline struct mtk_drm_crtc *to_mtk_crtc(struct drm_crtc *c)
{
	return container_of(c, struct mtk_drm_crtc, base);
}

void mtk_crtc_layer_config(unsigned int idx, struct mtk_plane_state *state,
				struct drm_crtc *crtc)
{
	struct mtk_drm_crtc *mtk_crtc;
	struct mtk_ddp_comp *ovl;

	if (crtc == NULL)
		crtc = state->base.crtc;

	mtk_crtc = to_mtk_crtc(crtc);
	ovl = mtk_crtc->ddp_comp[0];

	if (mtk_crtc->do_shadow_reg)
		mtk_ddp_comp_layer_config(ovl, idx, state);
}

void mtk_crtc_layer_off(unsigned int idx, struct mtk_plane_state *state,
				struct drm_crtc *crtc)
{
	struct mtk_drm_crtc *mtk_crtc;
	struct mtk_ddp_comp *ovl;

	if (crtc == NULL)
		crtc = state->base.crtc;

	mtk_crtc = to_mtk_crtc(crtc);
	ovl = mtk_crtc->ddp_comp[0];

	if (mtk_crtc->do_shadow_reg)
		mtk_ddp_comp_layer_off(ovl, idx);
}

void mtk_crtc_layer_on(unsigned int idx, struct mtk_plane_state *state,
				struct drm_crtc *crtc)
{
	struct mtk_drm_crtc *mtk_crtc;
	struct mtk_ddp_comp *ovl;

	if (crtc == NULL)
		crtc = state->base.crtc;

	mtk_crtc = to_mtk_crtc(crtc);
	ovl = mtk_crtc->ddp_comp[0];

	if (mtk_crtc->do_shadow_reg)
		mtk_ddp_comp_layer_on(ovl, idx);
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
	drm_crtc_cleanup(crtc);
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
	/* drm framework doesn't check NULL */
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
	state->pending_config = true;

	if (mtk_crtc->do_shadow_reg)
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

static void mtk_crtc_ddp_power_on(struct mtk_drm_crtc *mtk_crtc)
{
	int ret;
	int i;

	DRM_INFO("mtk_crtc_ddp_power_on\n");
	for (i = 0; i < mtk_crtc->ddp_comp_nr; i++) {
		ret = clk_enable(mtk_crtc->ddp_comp[i]->clk);
		if (ret)
			DRM_ERROR("Failed to enable clock %d: %d\n", i, ret);
	}
}

static void mtk_crtc_ddp_power_off(struct mtk_drm_crtc *mtk_crtc)
{
	int i;

	DRM_INFO("mtk_crtc_ddp_power_off\n");
	for (i = 0; i < mtk_crtc->ddp_comp_nr; i++)
		clk_disable(mtk_crtc->ddp_comp[i]->clk);
}

static void mtk_crtc_ddp_hw_init(struct mtk_drm_crtc *mtk_crtc)
{
	struct drm_crtc *crtc = &mtk_crtc->base;
	unsigned int width, height, vrefresh;
	int ret;
	int i;

	if (crtc->state) {
		width = crtc->state->adjusted_mode.hdisplay;
		height = crtc->state->adjusted_mode.vdisplay;
		vrefresh = crtc->state->adjusted_mode.vrefresh;
	} else {
		WARN_ON(true);
		width = 1920;
		height = 1080;
		vrefresh = 60;
	}

	DRM_INFO("mtk_crtc_ddp_hw_init\n");

	/* disp_mtcmos */
	ret = pm_runtime_get_sync(crtc->dev->dev);
	if (ret < 0)
		DRM_ERROR("Failed to enable power domain: %d\n", ret);

	mtk_disp_mutex_prepare(mtk_crtc->mutex);
	mtk_crtc_ddp_power_on(mtk_crtc);

	DRM_INFO("mediatek_ddp_ddp_path_setup\n");
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

	DRM_INFO("ddp_disp_path_power_on %dx%d\n", width, height);
	for (i = 0; i < mtk_crtc->ddp_comp_nr; i++) {
		struct mtk_ddp_comp *comp = mtk_crtc->ddp_comp[i];

		mtk_ddp_comp_config(comp, width, height, vrefresh);
		mtk_ddp_comp_power_on(comp);
	}
}

static void mtk_crtc_ddp_hw_fini(struct mtk_drm_crtc *mtk_crtc)
{
	struct drm_device *drm = mtk_crtc->base.dev;
	int i;

	DRM_INFO("mtk_crtc_ddp_hw_fini\n");
	mtk_crtc_ddp_power_off(mtk_crtc);
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
	mtk_disp_mutex_unprepare(mtk_crtc->mutex);

	/* disp_mtcmos */
	pm_runtime_put(drm->dev);
}

static void mtk_drm_crtc_enable(struct drm_crtc *crtc)
{
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(crtc);
	struct mtk_ddp_comp *ovl = mtk_crtc->ddp_comp[0];
	int ret;

	DRM_INFO("mtk_drm_crtc_enable %d\n", crtc->base.id);

	ret = mtk_smi_larb_get(ovl->larb_dev);
	if (ret)
		DRM_ERROR("Failed to get larb: %d\n", ret);

	mtk_crtc_ddp_hw_init(mtk_crtc);

	drm_crtc_vblank_on(crtc);
	WARN_ON(drm_crtc_vblank_get(crtc) != 0);
}

static void mtk_drm_crtc_disable(struct drm_crtc *crtc)
{
	struct mtk_drm_crtc *mtk_crtc = to_mtk_crtc(crtc);
	struct mtk_crtc_state *state = to_mtk_crtc_state(crtc->state);
	struct mtk_ddp_comp *ovl = mtk_crtc->ddp_comp[0];

	DRM_INFO("mtk_drm_crtc_disable %d\n", crtc->base.id);

	/*
	 * Before mtk_drm_crtc_disable is called by disable_outputs from
	 * drm_atomic_helper_commit_modeset_disables, the same function already
	 * called mtk_dsi/dpi_encoder_disable, which currently disables the
	 * pixel clock, and caused the pipeline to halt immediately.
	 * There will be no more OVL frame completion (vblank) interrupt.
	 *
	 * Ideally, the disabling of the pixel clock should be postponed
	 * until vblank, this function would just set all pending plane
	 * state to disabled and wait for the next vblank here.
	 */
	mtk_crtc_ddp_hw_fini(mtk_crtc);

	mtk_smi_larb_put(ovl->larb_dev);

	/* This will be called from the vblank interrupt in the future */
	mtk_drm_finish_page_flip(mtk_crtc);
	mtk_crtc->do_flush = false;
	if (state->event)
		mtk_drm_crtc_finish_page_flip(mtk_crtc);
	drm_crtc_vblank_put(crtc);
	drm_crtc_vblank_off(crtc);
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
	unsigned int i;

	for (i = 0; i < OVL_LAYER_NR; i++) {
		struct drm_plane *plane = &mtk_crtc->planes[i].base;
		struct mtk_plane_state *plane_state;

		plane_state = to_mtk_plane_state(plane->state);
		if (plane_state->pending_dirty) {
			plane_state->pending_config = true;
			plane_state->pending_dirty = false;
		}
	}

	if (mtk_crtc->do_shadow_reg) {
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
	.reset			= drm_atomic_helper_crtc_reset,
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
		struct drm_plane *cursor, int pipe)
{
	struct mtk_drm_private *priv = drm->dev_private;
	int ret;

	priv->crtc[pipe] = &mtk_crtc->base;

	ret = drm_crtc_init_with_planes(drm, &mtk_crtc->base, primary, cursor,
			&mtk_crtc_funcs);
	if (ret)
		goto err_cleanup_crtc;

	drm_crtc_helper_add(&mtk_crtc->base, &mtk_crtc_helper_funcs);

	return 0;

err_cleanup_crtc:
	drm_crtc_cleanup(&mtk_crtc->base);
	return ret;
}

static void mtk_crtc_ddp_irq(struct mtk_drm_crtc *mtk_crtc)
{
	struct mtk_ddp_comp *ovl = mtk_crtc->ddp_comp[0];
	struct mtk_crtc_state *state = to_mtk_crtc_state(mtk_crtc->base.state);
	unsigned int i;

	/*
	 * TODO: instead of updating the registers here, we should prepare
	 * working registers in atomic_commit and let the hardware command
	 * queue update module registers on vblank.
	 */
	if (!mtk_crtc->do_shadow_reg) {
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

			if (plane_state->pending_config) {
				if (!plane_state->pending_enable)
					mtk_ddp_comp_layer_off(ovl, i);

				mtk_ddp_comp_layer_config(ovl, i, plane_state);

				if (plane_state->pending_enable)
					mtk_ddp_comp_layer_on(ovl, i);

				plane_state->pending_config = false;
			}
		}
	}

	mtk_drm_finish_page_flip(mtk_crtc);
}

static irqreturn_t mtk_disp_ovl_irq_handler(int irq, void *dev_id)
{
	struct mtk_disp_ovl *priv = dev_id;
	struct mtk_drm_crtc *mtk_crtc = priv->crtc;
	struct mtk_ddp_comp *ovl = &priv->ddp_comp;

	mtk_ddp_comp_clear_vblank(ovl);

	if (mtk_crtc->pipe >= 0 && mtk_crtc->base.dev)
		mtk_crtc_ddp_irq(mtk_crtc);

	return IRQ_HANDLED;
}

static int mtk_disp_ovl_bind(struct device *dev, struct device *master,
		void *data)
{
	struct mtk_disp_ovl *priv = dev_get_drvdata(dev);
	struct drm_device *drm_dev = data;
	struct mtk_drm_private *drm_priv = drm_dev->dev_private;
	struct mtk_drm_crtc *mtk_crtc;
	int pipe = drm_priv->num_pipes;
	enum mtk_ddp_comp_id comp_id;
	struct device_node *node;
	enum drm_plane_type type;
	unsigned int zpos;
	int ret;
	int i;

	ret = mtk_ddp_comp_register(drm_dev, &priv->ddp_comp);
	if (ret < 0) {
		dev_err(dev, "Failed to register component %s: %d\n",
			dev->of_node->full_name, ret);
		return ret;
	}

	mtk_crtc = devm_kzalloc(drm_dev->dev, sizeof(*mtk_crtc), GFP_KERNEL);
	if (!mtk_crtc)
		return -ENOMEM;
	mtk_crtc->do_shadow_reg = of_property_read_bool(dev->of_node,
							"shadow_register");
	mtk_crtc->config_regs = drm_priv->config_regs;
	mtk_crtc->ddp_comp_nr = drm_priv->path_len[pipe];
	mtk_crtc->ddp_comp = devm_kmalloc_array(dev, mtk_crtc->ddp_comp_nr,
						sizeof(*mtk_crtc->ddp_comp),
						GFP_KERNEL);

	mtk_crtc->mutex = mtk_disp_mutex_get(drm_priv->mutex_dev, pipe);
	if (IS_ERR(mtk_crtc->mutex)) {
		ret = PTR_ERR(mtk_crtc->mutex);
		dev_err(dev, "Failed to get mutex: %d\n", ret);
		return ret;
	}

	comp_id = drm_priv->path[pipe][0];

	for (i = 0; i < mtk_crtc->ddp_comp_nr; i++) {
		struct mtk_ddp_comp *comp;

		comp_id = drm_priv->path[pipe][i];
		comp = drm_priv->ddp_comp[comp_id];
		if (!comp) {
			dev_err(dev, "Component %s not initialized\n",
				drm_priv->comp_node[comp_id]->full_name);
			ret = -ENODEV;
			goto unprepare;
		}

		ret = clk_prepare(comp->clk);
		if (ret) {
			dev_err(dev,
				"Failed to prepare clock for component %s: %d\n",
				drm_priv->comp_node[comp_id]->full_name, ret);
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

	priv->crtc = mtk_crtc;
	drm_priv->num_pipes++;

	for (i = 0; i < OVL_LAYER_NR; i++)
		mtk_ddp_comp_layer_off(&priv->ddp_comp, i);

	return 0;

unprepare:
	while (--i >= 0)
		clk_unprepare(mtk_crtc->ddp_comp[i]->clk);
	of_node_put(node);

	return ret;
}

static void mtk_disp_ovl_unbind(struct device *dev, struct device *master,
		void *data)
{
	struct mtk_disp_ovl *priv = dev_get_drvdata(dev);
	struct drm_device *drm_dev = data;
	struct mtk_drm_crtc *mtk_crtc = priv->crtc;
	int i;

	for (i = 0; i < mtk_crtc->ddp_comp_nr; i++)
		clk_unprepare(mtk_crtc->ddp_comp[i]->clk);

	mtk_ddp_comp_unregister(drm_dev, &priv->ddp_comp);
	mtk_disp_mutex_put(mtk_crtc->mutex);
}

static const struct component_ops mtk_disp_ovl_component_ops = {
	.bind	= mtk_disp_ovl_bind,
	.unbind = mtk_disp_ovl_unbind,
};

static struct mtk_ddp_comp_driver_data mt8173_ovl_driver_data = {
	.reg_ovl_addr = 0x0f40,
	.ovl_infmt_rgb565 = 0,
	.ovl_infmt_rgb888 = 1,
};

static struct mtk_ddp_comp_driver_data mt2701_ovl_driver_data = {
	.reg_ovl_addr = 0x0040,
	.ovl_infmt_rgb565 = 1,
	.ovl_infmt_rgb888 = 0,
};

static const struct of_device_id mtk_disp_ovl_driver_dt_match[] = {
	{ .compatible = "mediatek,mt2701-disp-ovl",
	  .data = &mt2701_ovl_driver_data},
	{ .compatible = "mediatek,mt8173-disp-ovl",
	  .data = &mt8173_ovl_driver_data},
	{},
};

static inline struct mtk_ddp_comp_driver_data *mtk_ovl_get_driver_data(
	struct platform_device *pdev)
{
	const struct of_device_id *of_id =
		of_match_device(mtk_disp_ovl_driver_dt_match, &pdev->dev);

	return (struct mtk_ddp_comp_driver_data *)of_id->data;
}

static int mtk_disp_ovl_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_disp_ovl *priv;
	int comp_id;
	int irq;
	int ret;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;

	ret = devm_request_irq(dev, irq, mtk_disp_ovl_irq_handler,
			       IRQF_TRIGGER_NONE, dev_name(dev), priv);
	if (ret < 0) {
		dev_err(dev, "Failed to request irq %d: %d\n", irq, ret);
		return ret;
	}

	comp_id = mtk_ddp_comp_get_id(dev->of_node, MTK_DISP_OVL);
	if (comp_id < 0) {
		dev_err(dev, "Failed to identify by alias: %d\n", comp_id);
		return comp_id;
	}
	priv->ddp_comp.ddp_comp_driver_data = mtk_ovl_get_driver_data(pdev);

	ret = mtk_ddp_comp_init(dev, dev->of_node, &priv->ddp_comp, comp_id);
	if (ret) {
		dev_err(dev, "Failed to initialize component: %d\n", ret);
		return ret;
	}

	platform_set_drvdata(pdev, priv);

	ret = component_add(dev, &mtk_disp_ovl_component_ops);
	if (ret)
		dev_err(dev, "Failed to add component: %d\n", ret);

	return ret;
}

static int mtk_disp_ovl_remove(struct platform_device *pdev)
{
	component_del(&pdev->dev, &mtk_disp_ovl_component_ops);

	return 0;
}

MODULE_DEVICE_TABLE(of, mtk_disp_ovl_driver_dt_match);

struct platform_driver mtk_disp_ovl_driver = {
	.probe		= mtk_disp_ovl_probe,
	.remove		= mtk_disp_ovl_remove,
	.driver		= {
		.name	= "mediatek-disp-ovl",
		.owner	= THIS_MODULE,
		.of_match_table = mtk_disp_ovl_driver_dt_match,
	},
};

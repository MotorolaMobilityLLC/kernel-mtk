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

#ifndef MTK_DRM_CRTC_H
#define MTK_DRM_CRTC_H

#include <drm/drm_crtc.h>
#include "mtk_drm_plane.h"

#define OVL_LAYER_NR	4

struct mtk_crtc_state {
	struct drm_crtc_state		base;
	struct drm_pending_vblank_event	*event;

	bool				pending_needs_vblank;

	bool				pending_config;
	unsigned int			pending_width;
	unsigned int			pending_height;
	unsigned int			pending_vrefresh;
};

static inline struct mtk_crtc_state *to_mtk_crtc_state(struct drm_crtc_state *s)
{
	return container_of(s, struct mtk_crtc_state, base);
}

int mtk_drm_crtc_enable_vblank(struct drm_device *drm, unsigned int pipe);
void mtk_drm_crtc_disable_vblank(struct drm_device *drm, unsigned int pipe);
void mtk_drm_crtc_check_flush(struct drm_crtc *crtc);
void mtk_drm_crtc_commit(struct drm_crtc *crtc);
void mtk_crtc_layer_config(unsigned int idx, struct mtk_plane_state *state,
				struct drm_crtc *crtc);
void mtk_crtc_layer_on(unsigned int idx, struct mtk_plane_state *state,
				struct drm_crtc *crtc);
void mtk_crtc_layer_off(unsigned int idx, struct mtk_plane_state *state,
				struct drm_crtc *crtc);

#endif /* MTK_DRM_CRTC_H */

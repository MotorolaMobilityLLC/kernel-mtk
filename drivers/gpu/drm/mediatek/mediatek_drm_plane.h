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

#ifndef _MEDIATEK_DRM_PLANE_H_
#define _MEDIATEK_DRM_PLANE_H_

#include "mediatek_drm_crtc.h"

#define to_mediatek_plane(x)	container_of(x, struct mediatek_drm_plane, base)

struct mediatek_drm_plane {
	struct drm_plane base;
	unsigned int src_x;
	unsigned int src_y;
	unsigned int src_width;
	unsigned int src_height;
	unsigned int fb_width;
	unsigned int fb_height;
	unsigned int crtc_x;
	unsigned int crtc_y;
	unsigned int crtc_width;
	unsigned int crtc_height;
	unsigned int mode_width;
	unsigned int mode_height;
	unsigned int h_ratio;
	unsigned int v_ratio;
	unsigned int refresh;
	unsigned int scan_flag;
	unsigned int bpp;
	unsigned int pitch;
	uint32_t pixel_format;
	dma_addr_t dma_addr;
	unsigned int zpos;
	unsigned int index_color;

	bool default_win:1;
	bool color_key:1;
	bool local_path:1;
	bool transparency:1;
	bool activated:1;
	bool enabled:1;
	bool resume:1;
};

void mediatek_plane_finish_page_flip(struct mtk_drm_crtc *mtk_crtc);
int mediatek_plane_init(struct drm_device *dev,
		      struct mediatek_drm_plane *mediatek_plane,
		      unsigned long possible_crtcs, enum drm_plane_type type,
		      unsigned int zpos, unsigned int max_plane);

#endif

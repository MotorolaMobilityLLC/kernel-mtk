/* mediatek_drm_dmabuf.h
 *
 * Copyright (c) 2015 MediaTek Inc.
 * Author: CK Hu <ck.hu@mediatek.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef _MEDIATEK_DRM_DMABUF_H_
#define _MEDIATEK_DRM_DMABUF_H_


struct dma_buf *mtk_dmabuf_prime_export(struct drm_device *drm_dev,
				struct drm_gem_object *obj, int flags);

struct drm_gem_object *mtk_dmabuf_prime_import(struct drm_device *drm_dev,
						struct dma_buf *dma_buf);
#endif


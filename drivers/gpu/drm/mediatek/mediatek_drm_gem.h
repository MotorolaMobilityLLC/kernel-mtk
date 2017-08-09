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

#ifndef _MEDIATEK_DRM_GEM_H_
#define _MEDIATEK_DRM_GEM_H_

#include <drm/drm_gem.h>

struct drm_gem_object;

/*
 * mtk drm buffer structure.
 *
 * @base: a gem object.
 *	- a new handle to this gem object would be created
 *	by drm_gem_handle_create().
 * @buffer: a pointer to mtk_drm_gem_buffer object.
 *	- contain the information to memory region allocated
 *	by user request or at framebuffer creation.
 *	continuous memory region allocated by user request
 *	or at framebuffer creation.
 * @size: total memory size to physically non-continuous memory region.
 * @flags: indicate memory type to allocated buffer and cache attruibute.
 *
 * P.S. this object would be transferred to user as kms_bo.handle so
 *	user can access the buffer through kms_bo.handle.
 */
struct mtk_drm_gem_obj {
	struct drm_gem_object	base;
	void __iomem		*kvaddr;
	dma_addr_t		dma_addr;
	struct dma_attrs	dma_attrs;
	struct sg_table		*sgt;
};

#define to_mtk_gem_obj(x)	container_of(x, struct mtk_drm_gem_obj, base)

struct mtk_drm_gem_obj *mtk_drm_gem_init(struct drm_device *dev,
						      unsigned long size);
void mtk_drm_gem_free_object(struct drm_gem_object *gem);
struct mtk_drm_gem_obj *mtk_drm_gem_create(struct drm_device *dev,
		unsigned long size, bool alloc_kmap);
int mtk_drm_gem_dumb_create(struct drm_file *file_priv,
		struct drm_device *dev,	struct drm_mode_create_dumb *args);
int mtk_drm_gem_dumb_map_offset(struct drm_file *file_priv,
		struct drm_device *dev, uint32_t handle, uint64_t *offset);
int mtk_drm_gem_mmap(struct file *filp, struct vm_area_struct *vma);
int mtk_drm_gem_mmap_buf(struct drm_gem_object *obj,
               struct vm_area_struct *vma);
/*
 * request gem object creation and buffer allocation as the size
 * that it is calculated with framebuffer information such as width,
 * height and bpp.
 */
int mediatek_gem_create_ioctl(struct drm_device *dev, void *data,
			      struct drm_file *file_priv);

/* get buffer offset to map to user space. */
int mediatek_gem_map_offset_ioctl(struct drm_device *dev, void *data,
				  struct drm_file *file_priv);


#endif


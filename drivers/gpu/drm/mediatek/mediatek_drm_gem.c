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
#include <drm/mediatek_drm.h>

#include "mediatek_drm_gem.h"

struct mtk_drm_gem_obj *mtk_drm_gem_init(struct drm_device *dev,
						      unsigned long size)
{
	struct mtk_drm_gem_obj *mtk_gem_obj;
	struct drm_gem_object *obj;
	int ret;

	size = round_up(size, PAGE_SIZE);

	mtk_gem_obj = kzalloc(sizeof(*mtk_gem_obj), GFP_KERNEL);
	if (!mtk_gem_obj)
		return ERR_PTR(-ENOMEM);

	obj = &mtk_gem_obj->base;

	ret = drm_gem_object_init(dev, obj, size);
	if (ret < 0) {
		DRM_ERROR("failed to initialize gem object\n");
		kfree(mtk_gem_obj);
		return ERR_PTR(ret);
	}

	return mtk_gem_obj;
}

struct mtk_drm_gem_obj *mtk_drm_gem_create(struct drm_device *dev,
		unsigned long size, bool alloc_kmap)
{
	struct mtk_drm_gem_obj *mtk_gem;
	struct drm_gem_object *obj;
	int ret;

	mtk_gem = mtk_drm_gem_init(dev, size);
	if (IS_ERR(mtk_gem))
		return ERR_CAST(mtk_gem);

	obj = &mtk_gem->base;

	init_dma_attrs(&mtk_gem->dma_attrs);
	dma_set_attr(DMA_ATTR_WRITE_COMBINE, &mtk_gem->dma_attrs);

	if (!alloc_kmap)
		dma_set_attr(DMA_ATTR_NO_KERNEL_MAPPING, &mtk_gem->dma_attrs);

	mtk_gem->kvaddr = dma_alloc_attrs(dev->dev, obj->size,
			(dma_addr_t *)&mtk_gem->dma_addr, GFP_KERNEL,
			&mtk_gem->dma_attrs);
	if (!mtk_gem->kvaddr) {
		DRM_ERROR("failed to allocate %zx byte dma buffer", obj->size);
		ret = -ENOMEM;
		goto err_dma;
	}

	{
		mtk_gem->sgt = kzalloc(sizeof(*mtk_gem->sgt), GFP_KERNEL);
		if (!mtk_gem->sgt) {
			ret = -ENOMEM;
			goto err_sgt;
		}

		ret = dma_get_sgtable_attrs(dev->dev, mtk_gem->sgt,
			mtk_gem->kvaddr, mtk_gem->dma_addr, obj->size,
			&mtk_gem->dma_attrs);
		if (ret) {
			DRM_ERROR("failed to allocate sgt, %d\n", ret);
			goto err_get_sgtable;
		}
	}

	DRM_INFO("kvaddr = %p dma_addr = %pad\n",
			mtk_gem->kvaddr, &mtk_gem->dma_addr);

	return mtk_gem;
err_get_sgtable:
	kfree(mtk_gem->sgt);
err_sgt:
	dma_free_attrs(dev->dev, obj->size, mtk_gem->kvaddr, mtk_gem->dma_addr,
			&mtk_gem->dma_attrs);
err_dma:
	kfree(mtk_gem);
	return ERR_PTR(ret);
}

void mtk_drm_gem_free_object(struct drm_gem_object *obj)
{
	struct mtk_drm_gem_obj *mtk_gem = to_mtk_gem_obj(obj);

	drm_gem_free_mmap_offset(obj);

	/* release file pointer to gem object. */
	drm_gem_object_release(obj);

	dma_free_attrs(obj->dev->dev, obj->size, mtk_gem->kvaddr,
			mtk_gem->dma_addr, &mtk_gem->dma_attrs);

	kfree(mtk_gem);
}

int mtk_drm_gem_dumb_create(struct drm_file *file_priv,
			       struct drm_device *dev,
			       struct drm_mode_create_dumb *args)

{
	struct mtk_drm_gem_obj *mtk_gem;
	unsigned int min_pitch = args->width * ((args->bpp + 7) / 8);
	int ret;

	args->pitch = min_pitch;
	args->size = args->pitch * args->height;

	mtk_gem = mtk_drm_gem_create(dev, args->size, false);
	if (IS_ERR(mtk_gem))
		return PTR_ERR(mtk_gem);

	/*
	 * allocate a id of idr table where the obj is registered
	 * and handle has the id what user can see.
	 */
	ret = drm_gem_handle_create(file_priv, &mtk_gem->base, &args->handle);
	if (ret)
		goto err_handle_create;

	/* drop reference from allocate - handle holds it now. */
	drm_gem_object_unreference_unlocked(&mtk_gem->base);

	return 0;

err_handle_create:
	mtk_drm_gem_free_object(&mtk_gem->base);
	return ret;
}

int mtk_drm_gem_dumb_map_offset(struct drm_file *file_priv,
				   struct drm_device *dev, uint32_t handle,
				   uint64_t *offset)
{
	struct drm_gem_object *obj;
	int ret;

	mutex_lock(&dev->struct_mutex);

	obj = drm_gem_object_lookup(dev, file_priv, handle);
	if (!obj) {
		DRM_ERROR("failed to lookup gem object.\n");
		ret = -EINVAL;
		goto unlock;
	}

	ret = drm_gem_create_mmap_offset(obj);
	if (ret)
		goto out;

	*offset = drm_vma_node_offset_addr(&obj->vma_node);
	DRM_DEBUG_KMS("offset = 0x%llx\n", *offset);

out:
	drm_gem_object_unreference(obj);
unlock:
	mutex_unlock(&dev->struct_mutex);
	return ret;
}

int mtk_drm_gem_mmap_buf(struct drm_gem_object *obj,
		struct vm_area_struct *vma)
{
	struct mtk_drm_gem_obj *mtk_gem = to_mtk_gem_obj(obj);
	struct drm_device *drm = obj->dev;
	unsigned long vm_size;

	vma->vm_flags |= VM_IO | VM_DONTEXPAND | VM_DONTDUMP;
	vm_size = vma->vm_end - vma->vm_start;

	if (vm_size > obj->size)
		return -EINVAL;

	return dma_mmap_attrs(drm->dev, vma, mtk_gem->kvaddr, mtk_gem->dma_addr,
			obj->size, &mtk_gem->dma_attrs);
}

int mtk_drm_gem_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct drm_file *file_priv = filp->private_data;
	struct drm_device *dev = file_priv->minor->dev;
	struct drm_gem_object *obj;
	struct drm_vma_offset_node *node;
	int ret;

	if (drm_device_is_unplugged(dev))
		return -ENODEV;

	mutex_lock(&dev->struct_mutex);

	node = drm_vma_offset_exact_lookup(dev->vma_offset_manager,
				   vma->vm_pgoff,
				   vma_pages(vma));
	if (!node) {
		mutex_unlock(&dev->struct_mutex);
		DRM_ERROR("failed to find vma node.\n");
		return -EINVAL;
	} else if (!drm_vma_node_is_allowed(node, filp)) {
		mutex_unlock(&dev->struct_mutex);
		return -EACCES;
	}

	/*
	 * Set vm_pgoff (used as a fake buffer offset by DRM) to 0 and map the
	 * whole buffer from the start.
	 */
	vma->vm_pgoff = 0;

	obj = container_of(node, struct drm_gem_object, vma_node);
	ret = mtk_drm_gem_mmap_buf(obj, vma);

	mutex_unlock(&dev->struct_mutex);

	return ret;
}

int mediatek_gem_map_offset_ioctl(struct drm_device *drm, void *data,
				  struct drm_file *file_priv)
{
	struct drm_mtk_gem_map_off *args = data;

	return mtk_drm_gem_dumb_map_offset(file_priv, drm, args->handle,
						&args->offset);
}

int mediatek_gem_create_ioctl(struct drm_device *dev, void *data,
			      struct drm_file *file_priv)
{
	struct mtk_drm_gem_obj *mtk_gem;
	struct drm_mtk_gem_create *args = data;
	int ret;

	mtk_gem = mtk_drm_gem_create(dev, args->size, false);
	if (IS_ERR(mtk_gem))
		return PTR_ERR(mtk_gem);

	/*
	 * allocate a id of idr table where the obj is registered
	 * and handle has the id what user can see.
	 */
	ret = drm_gem_handle_create(file_priv, &mtk_gem->base, &args->handle);
	if (ret)
		goto err_handle_create;

	/* drop reference from allocate - handle holds it now. */
	drm_gem_object_unreference_unlocked(&mtk_gem->base);

	return 0;

err_handle_create:
	mtk_drm_gem_free_object(&mtk_gem->base);
	return ret;
}


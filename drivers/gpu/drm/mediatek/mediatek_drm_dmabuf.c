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

#include "mediatek_drm_gem.h"


struct mtk_drm_dmabuf_attachment {
	struct sg_table sgt;
	enum dma_data_direction dir;
	bool is_mapped;
};

static struct mtk_drm_gem_obj *dma_buf_to_obj(struct dma_buf *buf)
{
	return to_mtk_gem_obj(buf->priv);
}

static int mtk_gem_attach_dma_buf(struct dma_buf *dmabuf,
					struct device *dev,
					struct dma_buf_attachment *attach)
{
	struct mtk_drm_dmabuf_attachment *mtk_attach;

	mtk_attach = kzalloc(sizeof(*mtk_attach), GFP_KERNEL);
	if (!mtk_attach)
		return -ENOMEM;

	mtk_attach->dir = DMA_NONE;
	attach->priv = mtk_attach;

	return 0;
}

static void mtk_gem_detach_dma_buf(struct dma_buf *dmabuf,
					struct dma_buf_attachment *attach)
{
	struct mtk_drm_dmabuf_attachment *mtk_attach = attach->priv;
	struct sg_table *sgt;

	if (!mtk_attach)
		return;

	sgt = &mtk_attach->sgt;

	if (mtk_attach->dir != DMA_NONE)
		dma_unmap_sg(attach->dev, sgt->sgl, sgt->nents,
				mtk_attach->dir);

	sg_free_table(sgt);
	kfree(mtk_attach);
	attach->priv = NULL;
}

static struct sg_table *
		mtk_gem_map_dma_buf(struct dma_buf_attachment *attach,
					enum dma_data_direction dir)
{
	struct mtk_drm_dmabuf_attachment *mtk_attach = attach->priv;
	struct mtk_drm_gem_obj *gem_obj = dma_buf_to_obj(attach->dmabuf);
	struct drm_device *dev = gem_obj->base.dev;
	struct scatterlist *rd, *wr;
	struct sg_table *sgt = NULL;
	unsigned int i;
	int nents, ret;

	/* just return current sgt if already requested. */
	if (mtk_attach->dir == dir && mtk_attach->is_mapped)
		return &mtk_attach->sgt;

	sgt = &mtk_attach->sgt;

	ret = sg_alloc_table(sgt, gem_obj->sgt->orig_nents, GFP_KERNEL);
	if (ret) {
		DRM_ERROR("failed to alloc sgt.\n");
		return ERR_PTR(-ENOMEM);
	}

	mutex_lock(&dev->struct_mutex);

	rd = gem_obj->sgt->sgl;
	wr = sgt->sgl;
	for (i = 0; i < sgt->orig_nents; ++i) {
		sg_set_page(wr, sg_page(rd), rd->length, rd->offset);
		rd = sg_next(rd);
		wr = sg_next(wr);
	}

	if (dir != DMA_NONE) {
		nents = dma_map_sg(attach->dev, sgt->sgl, sgt->orig_nents, dir);
		if (!nents) {
			DRM_ERROR("failed to map sgl with iommu.\n");
			sg_free_table(sgt);
			sgt = ERR_PTR(-EIO);
			goto err_unlock;
		}
	}

	mtk_attach->is_mapped = true;
	mtk_attach->dir = dir;
	attach->priv = mtk_attach;

	DRM_DEBUG_PRIME("buffer size = 0x%zx\n", attach->dmabuf->size);

err_unlock:
	mutex_unlock(&dev->struct_mutex);
	return sgt;
}

static void mtk_gem_unmap_dma_buf(struct dma_buf_attachment *attach,
						struct sg_table *sgt,
						enum dma_data_direction dir)
{
	/* Nothing to do. */
}

static void *mtk_gem_dmabuf_kmap_atomic(struct dma_buf *dma_buf,
						unsigned long page_num)
{
	/* TODO */

	return NULL;
}

static void mtk_gem_dmabuf_kunmap_atomic(struct dma_buf *dma_buf,
						unsigned long page_num,
						void *addr)
{
	/* TODO */
}

static void *mtk_gem_dmabuf_kmap(struct dma_buf *dma_buf,
					unsigned long page_num)
{
	/* TODO */

	return NULL;
}

static void mtk_gem_dmabuf_kunmap(struct dma_buf *dma_buf,
					unsigned long page_num, void *addr)
{
	/* TODO */
}

static int mtk_gem_dmabuf_mmap(struct dma_buf *dma_buf,
	struct vm_area_struct *vma)
{
	return -ENOTTY;
}

static void *mtk_gem_dmabuf_vmap(struct dma_buf *buf)
{
	struct drm_gem_object *obj = buf->priv;
	struct mtk_drm_gem_obj *mtk_gem_obj = to_mtk_gem_obj(obj);

	return mtk_gem_obj->kvaddr;
}

static void mtk_gem_dmabuf_vunmap(struct dma_buf *buf, void *vaddr)
{
}


static struct dma_buf_ops mtk_dmabuf_ops = {
	.attach			= mtk_gem_attach_dma_buf,
	.detach			= mtk_gem_detach_dma_buf,
	.map_dma_buf		= mtk_gem_map_dma_buf,
	.unmap_dma_buf		= mtk_gem_unmap_dma_buf,
	.kmap			= mtk_gem_dmabuf_kmap,
	.kmap_atomic		= mtk_gem_dmabuf_kmap_atomic,
	.kunmap			= mtk_gem_dmabuf_kunmap,
	.kunmap_atomic		= mtk_gem_dmabuf_kunmap_atomic,
	.mmap			= mtk_gem_dmabuf_mmap,
	.release		= drm_gem_dmabuf_release,
	.vmap			= mtk_gem_dmabuf_vmap,
	.vunmap			= mtk_gem_dmabuf_vunmap,
};

struct dma_buf *mtk_dmabuf_prime_export(struct drm_device *drm_dev,
				struct drm_gem_object *obj, int flags)
{
	struct mtk_drm_gem_obj *mtk_gem_obj = to_mtk_gem_obj(obj);

	return dma_buf_export(obj, &mtk_dmabuf_ops,
				mtk_gem_obj->base.size, flags, NULL);
}

struct drm_gem_object *mtk_dmabuf_prime_import(struct drm_device *drm_dev,
				struct dma_buf *dma_buf)
{
	struct dma_buf_attachment *attach;
	struct sg_table *sgt;
	struct scatterlist *sgl;
	struct mtk_drm_gem_obj *mtk_gem_obj;
	int ret;

	/* is this one of own objects? */
	if (dma_buf->ops == &mtk_dmabuf_ops) {
		struct drm_gem_object *obj;

		obj = dma_buf->priv;

		/* is it from our device? */
		if (obj->dev == drm_dev) {
			/*
			 * Importing dmabuf exported from out own gem increases
			 * refcount on gem itself instead of f_count of dmabuf.
			 */
			drm_gem_object_reference(obj);
			return obj;
		}
	}

	attach = dma_buf_attach(dma_buf, drm_dev->dev);
	if (IS_ERR(attach))
		return ERR_PTR(-EINVAL);

	get_dma_buf(dma_buf);

	sgt = dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
	if (IS_ERR(sgt)) {
		ret = PTR_ERR(sgt);
		goto err_buf_detach;
	}

	mtk_gem_obj = mtk_drm_gem_init(drm_dev, dma_buf->size);
	if (!mtk_gem_obj) {
		ret = -ENOMEM;
		goto err_unmap_attach;
	}

	sgl = sgt->sgl;

	mtk_gem_obj->dma_addr = sg_dma_address(sgl);

	mtk_gem_obj->sgt = sgt;
	mtk_gem_obj->base.import_attach = attach;

	DRM_DEBUG_PRIME("dma_addr = %pad, size = 0x%zx\n",
			&mtk_gem_obj->dma_addr,	dma_buf->size);

	return &mtk_gem_obj->base;

err_unmap_attach:
	dma_buf_unmap_attachment(attach, sgt, DMA_BIDIRECTIONAL);
err_buf_detach:
	dma_buf_detach(dma_buf, attach);
	dma_buf_put(dma_buf);

	return ERR_PTR(ret);
}

MODULE_AUTHOR("CK Hu <ck.hu@mediatek.com>");
MODULE_DESCRIPTION("Mediatek SoC DRM driver");
MODULE_LICENSE("GPL");


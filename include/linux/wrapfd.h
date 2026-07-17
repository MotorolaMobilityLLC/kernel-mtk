/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * WrapFD kernel API
 *
 * Copyright (C) 2025 Google, Inc.
 */

#ifndef _LINUX_WRAPFD_H
#define _LINUX_WRAPFD_H

union wrapfd_mappable {
	struct dma_buf *dmabuf;
};

#ifdef CONFIG_ANDROID_WRAPFD

/* Check if the vma is mapping a wrapfd file. */
bool is_wrapfd_vma(struct vm_area_struct *vma);

/*
 * Get mappable object. Caller also gets buffer ownership.
 * Function does not take file references, therefore the caller should
 * ensure the file is stable until wrapfd_put_mappable() is called.
 *
 * file: wrap file to get the mappable object.
 * dev: device requesting the operation.
 * mappable: mappable object.
 *
 * On success returns mappable object. On error returns:
 * -EBADF: file is invalid.
 * -ENODEV: device is invalid.
 * -EINVAL: buffer is mapped.
 * -EBUSY: buffer is owned by another device.
 * -ENOENT: wrap is empty (buffer got freed or moved)
 */
int wrapfd_get_mappable(struct file *file, struct device *dev,
			union wrapfd_mappable *mappable);

/*
 * Release mappable object and buffer ownership. Caller should own the buffer.
 * Note that the caller might still keep the buffer mapped. If the buffer
 * is reused or freed then the next wrapfd_get_mappable() call will fail with
 * -ENOENT and the mapping should be invalidated. If wrapfd_get_mappable()
 * succeeds then previous mappings are still valid and can be reused.
 *
 * file: wrap file associated with the mappable object.
 * dev: device requesting the operation.
 * mappable: mappable object returned by wrapfd_get_mappable().
 *
 * On success returns 0. On error returns:
 * -EBADF: file is invalid.
 * -ENODEV: device is invalid.
 * -EBUSY: buffer is not owned by the caller.
 */
int wrapfd_put_mappable(struct file *file, struct device *dev,
			union wrapfd_mappable *mappable);

#else /* CONFIG_ANDROID_WRAPFD */

static inline bool is_wrapfd_vma(struct vm_area_struct *vma)
{
	return false;
}

static inline int wrapfd_get_mappable(struct file *file, struct device *dev,
				      union wrapfd_mappable *mappable)
{
	return -ENOENT;
}

static inline int wrapfd_put_mappable(struct file *file, struct device *dev,
				      union wrapfd_mappable *mappable)
{
	return -ENOENT;
}

#endif /* CONFIG_ANDROID_WRAPFD */

#endif /* _LINUX_WRAPFD_H */

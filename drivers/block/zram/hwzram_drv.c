/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * Modifications from MediaTek Inc.:
 * 1. Add "last" check of bio vectors in copied case of hwzram_bvec_read.
 * 2. Add bio_get before iterating bio vectors to avoid race condition.
 * 3. Add another work to handle pending free request to avoid deadlock.
 *
 * It is based on Hardware Compressed RAM block device (hwzram)
 *
 * Hardware Compressed RAM block device
 *
 *  Copyright (C) 2015 The Chromium OS Authors
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  Sonny Rao <sonnyrao@chromium.org>
 *
 * Based on Compressed RAM block device (zram)
 *
 * Copyright (C) 2008, 2009, 2010  Nitin Gupta
 *               2012, 2013 Minchan Kim
 *
 * This code is released using a dual license strategy: BSD/GPL
 * You can choose the licence that better fits your requirements.
 *
 * Released under the terms of 3-clause BSD License
 * Released under the terms of GNU General Public License Version 2.0
 *
 */

#define KMSG_COMPONENT "hwzram"
#define pr_fmt(fmt) KMSG_COMPONENT ": " fmt

#ifdef CONFIG_HWZRAM_DEBUG
#define DEBUG
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/bio.h>
#include <linux/bitops.h>
#include <linux/blkdev.h>
#include <linux/buffer_head.h>
#include <linux/device.h>
#include <linux/genhd.h>
#include <linux/highmem.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/err.h>
#include <linux/delay.h>

#include "hwzram_drv.h"

/* Globals */
static int hwzram_major;
static struct hwzram *hwzram_devices;

/* Module params (documentation at end) */
static unsigned int num_devices = 1;

#define HWZRAM_ATTR_RO(name) \
static ssize_t hwzram_attr_##name##_show(struct device *d,	\
						struct device_attribute *attr, char *b)	\
{									\
	struct hwzram *hwzram = dev_to_hwzram(d);				\
	return scnprintf(b, PAGE_SIZE, "%llu\n",			\
		(u64)atomic64_read(&hwzram->stats.name));			\
}									\
static struct device_attribute dev_attr_##name =			\
	__ATTR(name, S_IRUGO, hwzram_attr_##name##_show, NULL)

static inline int init_done(struct hwzram *hwzram)
{
	return hwzram->meta != NULL;
}

static inline struct hwzram *dev_to_hwzram(struct device *dev)
{
	return (struct hwzram *)dev_to_disk(dev)->private_data;
}

static ssize_t disksize_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct hwzram *hwzram = dev_to_hwzram(dev);

	return scnprintf(buf, PAGE_SIZE, "%llu\n", hwzram->disksize);
}

static ssize_t initstate_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u32 val;
	struct hwzram *hwzram = dev_to_hwzram(dev);

	down_read(&hwzram->init_lock);
	val = init_done(hwzram);
	up_read(&hwzram->init_lock);

	return scnprintf(buf, PAGE_SIZE, "%u\n", val);
}

static ssize_t orig_data_size_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct hwzram *hwzram = dev_to_hwzram(dev);

	return scnprintf(buf, PAGE_SIZE, "%llu\n",
		(u64)(atomic64_read(&hwzram->stats.pages_stored)) << PAGE_SHIFT);
}

static ssize_t mem_used_total_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u64 val = 0;
	struct hwzram *hwzram = dev_to_hwzram(dev);

	down_read(&hwzram->init_lock);
	if (init_done(hwzram)) {
		/* struct hwzram_meta *meta = hwzram->meta; */
		/* val = zs_get_total_pages(meta->mem_pool); */
		val = hwzram_impl_get_total_used(hwzram->hwz);
	}
	up_read(&hwzram->init_lock);

	return scnprintf(buf, PAGE_SIZE, "%llu\n", val);
}

static ssize_t mem_limit_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u64 val;
	struct hwzram *hwzram = dev_to_hwzram(dev);

	down_read(&hwzram->init_lock);
	val = hwzram->limit_pages;
	up_read(&hwzram->init_lock);

	return scnprintf(buf, PAGE_SIZE, "%llu\n", val << PAGE_SHIFT);
}

static ssize_t mem_limit_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	u64 limit;
	char *tmp;
	struct hwzram *hwzram = dev_to_hwzram(dev);

	limit = memparse(buf, &tmp);
	if (buf == tmp) /* no chars parsed, invalid input */
		return -EINVAL;

	down_write(&hwzram->init_lock);
	hwzram->limit_pages = PAGE_ALIGN(limit) >> PAGE_SHIFT;
	up_write(&hwzram->init_lock);

	return len;
}

static ssize_t mem_used_max_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u64 val = 0;
	struct hwzram *hwzram = dev_to_hwzram(dev);

	down_read(&hwzram->init_lock);
	if (init_done(hwzram))
		val = atomic_long_read(&hwzram->stats.max_used_pages);
	up_read(&hwzram->init_lock);

	return scnprintf(buf, PAGE_SIZE, "%llu\n", val << PAGE_SHIFT);
}

static ssize_t mem_used_max_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	int err;
	unsigned long val;
	struct hwzram *hwzram = dev_to_hwzram(dev);

	err = kstrtoul(buf, 10, &val);
	if (err || val != 0)
		return -EINVAL;

	down_read(&hwzram->init_lock);
	if (init_done(hwzram)) {
		/* struct hwzram_meta *meta = hwzram->meta; */
		/* atomic_long_set(&hwzram->stats.max_used_pages, */
		/*		zs_get_total_pages(meta->mem_pool)); */
	}
	up_read(&hwzram->init_lock);

	return len;
}

/* flag operations needs meta->tb_lock */
static int hwzram_test_flag(struct hwzram_meta *meta, u32 index,
			enum hwzram_pageflags flag)
{
	return meta->table[index].value & BIT(flag);
}

static void hwzram_set_flag(struct hwzram_meta *meta, u32 index,
			enum hwzram_pageflags flag)
{
	meta->table[index].value |= BIT(flag);
}

static void hwzram_clear_flag(struct hwzram_meta *meta, u32 index,
			enum hwzram_pageflags flag)
{
	meta->table[index].value &= ~BIT(flag);
}

static size_t hwzram_get_obj_size(struct hwzram_meta *meta, u32 index)
{
	return meta->table[index].value & (BIT(HWZRAM_FLAG_SHIFT) - 1);
}

static void hwzram_set_obj_size(struct hwzram_meta *meta,
					u32 index, size_t size)
{
	unsigned long flags = meta->table[index].value >> HWZRAM_FLAG_SHIFT;

	meta->table[index].value = (flags << HWZRAM_FLAG_SHIFT) | size;
}

static inline int is_partial_io(struct bio_vec *bvec)
{
	return bvec->bv_len != PAGE_SIZE;
}

/*
 * Check if request is within bounds and aligned on hwzram logical blocks.
 */
static inline int valid_io_request(struct hwzram *hwzram, struct bio *bio)
{
	u64 start, end, bound;

	/* unaligned request */
	if (unlikely(bio->bi_iter.bi_sector &
		     (HWZRAM_SECTOR_PER_LOGICAL_BLOCK - 1)))
		return 0;
	if (unlikely(bio->bi_iter.bi_size & (HWZRAM_LOGICAL_BLOCK_SIZE - 1)))
		return 0;

	start = bio->bi_iter.bi_sector;
	end = start + (bio->bi_iter.bi_size >> SECTOR_SHIFT);
	bound = hwzram->disksize >> SECTOR_SHIFT;
	/* out of range range */
	if (unlikely(start >= bound || end > bound || start > end))
		return 0;

	/* I/O request is valid */
	return 1;
}

static void hwzram_meta_free(struct hwzram_meta *meta)
{
	vfree(meta->table);
	kfree(meta);
}

static struct hwzram_meta *hwzram_meta_alloc(u64 disksize)
{
	size_t num_pages;
	struct hwzram_meta *meta = kmalloc(sizeof(*meta), GFP_KERNEL);

	if (!meta)
		goto out;

	num_pages = disksize >> PAGE_SHIFT;
	meta->table = vzalloc(num_pages * sizeof(*meta->table));
	if (!meta->table) {
		pr_err("Error allocating hwzram address table\n");
		goto free_meta;
	}

	return meta;

free_meta:
	kfree(meta);
	meta = NULL;
out:
	return meta;
}

static void update_position(u32 *index, int *offset, struct bio_vec *bvec)
{
	if (*offset + bvec->bv_len >= PAGE_SIZE)
		(*index)++;
	*offset = (*offset + bvec->bv_len) % PAGE_SIZE;
}

static void handle_zero_page(struct bio_vec *bvec)
{
	struct page *page = bvec->bv_page;
	void *user_mem;

	user_mem = kmap_atomic(page);
	if (is_partial_io(bvec))
		memset(user_mem + bvec->bv_offset, 0, bvec->bv_len);
	else
		clear_page(user_mem);
	kunmap_atomic(user_mem);

	flush_dcache_page(page);
}


/*
 * To protect concurrent access to the same index entry,
 * caller should hold this table index entry's bit_spinlock to
 * indicate this index entry is accessing.
 */
static void hwzram_free_page(struct hwzram *hwzram, size_t index)
{
	struct hwzram_meta *meta = hwzram->meta;

	if (hwzram_test_flag(meta, index, HWZRAM_DRV_ZERO)) {
		hwzram_clear_flag(meta, index, HWZRAM_DRV_ZERO);
		atomic64_dec(&hwzram->stats.zero_pages);
		atomic64_dec(&hwzram->stats.pages_stored);
		return;
	}

	if (hwzram_get_obj_size(meta, index)) {
		hwzram_impl_free_buffers(hwzram->hwz,
				 meta->table[index].compressed_buffers);
		memset(meta->table[index].compressed_buffers, 0,
		       sizeof(phys_addr_t) * HWZRAM_MAX_BUFFERS_USED);

		atomic64_sub(hwzram_get_obj_size(meta, index),
			     &hwzram->stats.compr_data_size);
		atomic64_dec(&hwzram->stats.pages_stored);
		hwzram_set_obj_size(meta, index, 0);
	}
}

static void hwzram_impl_decompr_cb(struct hwzram_impl_completion *cmpl)
{
	struct bio *bio = cmpl->data;
	u32 index = cmpl->index;
	unsigned int compr_size = cmpl->compr_size;
	struct hwzram *hwzram = cmpl->hwzram;
	struct hwzram_meta *meta = hwzram->meta;

	/* Do unlock here to avoid deadlock in bio_endio */
	bit_spin_unlock(HWZRAM_DRV_ACCESS, &meta->table[index].value);

	pr_devel("%s: index %u size %u cmpl->decompr_status %u flags %u\n",
		 __func__, index, compr_size, cmpl->decompr_status,
		 cmpl->flags);

	if (cmpl->decompr_status == DCMD_STATUS_DECOMPRESSED) {
		if (cmpl->flags)
			bio_endio(bio);
	}
	/* No need to handle bio_io_error for atomic decompression here.
	 * This will be finished in __hwzram_make_request.
	else {
		bio_io_error(bio);
	}
	*/

	kfree(cmpl);
}

static int hwzram_bvec_read(struct hwzram *hwzram, struct bio_vec *bvec,
			    u32 index, int offset, struct bio *bio,
			    bool last)
{
	int ret;
	struct page *page;
	struct hwzram_meta *meta = hwzram->meta;
	unsigned int i, buf_count, compr_size;
	bool copied = false;

	page = bvec->bv_page;

	bit_spin_lock(HWZRAM_DRV_ACCESS, &meta->table[index].value);
	compr_size = hwzram_get_obj_size(meta, index);
	if (hwzram_test_flag(meta, index, HWZRAM_DRV_ZERO) ||
	    !compr_size) {
		bit_spin_unlock(HWZRAM_DRV_ACCESS, &meta->table[index].value);
		handle_zero_page(bvec);
		if (last)
			bio_endio(bio);
		return 0;
	}

	/* if the size is set to UNCOMPRESSED_BLOCK_SIZE it's a copy */
	if (compr_size == UNCOMPRESSED_BLOCK_SIZE)
		copied = true;

	/* look through the buffers */
	buf_count = 0;
	for (i = 0; i < HWZRAM_MAX_BUFFERS_USED; i++) {
		if (meta->table[index].compressed_buffers[i])
			buf_count++;
	}
	pr_devel("%s: index %u offset %d buf_count %u size %zu copied %u\n",
		 __func__, index, offset, buf_count,
		 hwzram_get_obj_size(meta, index), copied);

	if (copied) {
		ret = hwzram_impl_copy_buf(hwzram->hwz, page,
				   meta->table[index].compressed_buffers);
		bit_spin_unlock(HWZRAM_DRV_ACCESS, &meta->table[index].value);
		/* Need to do bio_endio */
		if (last)
			bio_endio(bio);
	} else {
		struct hwzram_impl_completion *cmpl = kzalloc(sizeof(*cmpl),
							      GFP_ATOMIC);
		if (!cmpl) {
			ret = -ENOMEM;
			bit_spin_unlock(HWZRAM_DRV_ACCESS, &meta->table[index].value);
			goto out;
		}

		if (last)
			cmpl->flags = 1;
		cmpl->index = index;
		cmpl->decompr_status = DCMD_STATUS_IDLE;
		cmpl->hwzram = hwzram;
		cmpl->data = bio;
		cmpl->compr_size = hwzram_get_obj_size(meta, index);
		cmpl->callback = hwzram_impl_decompr_cb;
#ifdef CONFIG_MTK_ENG_BUILD
		cmpl->hash = meta->table[index].hash;
#endif
		memcpy(cmpl->dma_bufs, meta->table[index].compressed_buffers,
		       sizeof(phys_addr_t) * HWZRAM_MAX_BUFFERS_USED);
		ret = hwzram_impl_decompress_page(hwzram->hwz, page, cmpl);

		/* bit_spin_unlock will be done in hwzram_impl_decompr_cb */

		/* TBC - miss error handling */
		if (ret != 0)
			pr_err("%s: error %d\n", __func__, ret);
	}
out:
	return ret;
}

static inline void update_used_max(struct hwzram *hwzram,
					const unsigned long pages)
{
	int old_max, cur_max;

	old_max = atomic_long_read(&hwzram->stats.max_used_pages);

	do {
		cur_max = old_max;
		if (pages > cur_max)
			old_max = atomic_long_cmpxchg(
				&hwzram->stats.max_used_pages, cur_max, pages);
	} while (old_max != cur_max);
}

static void hwzram_impl_compr_cb(struct hwzram_impl_completion *cmpl)
{
	struct hwzram *hwzram = cmpl->hwzram;
	struct bio *bio = cmpl->data;
	struct hwzram_meta *meta = hwzram->meta;
	u32 index = cmpl->index;
	unsigned int compr_size = cmpl->compr_size;

	pr_devel("%s: index %u size %u cmpl->buffers[0] 0x%pap\n", __func__,
		 index, compr_size, &cmpl->buffers[0]);

	bit_spin_lock(HWZRAM_DRV_ACCESS, &meta->table[index].value);

	hwzram_free_page(hwzram, index);

	if (cmpl->compr_status == HWZRAM_ZERO) {
		hwzram_set_flag(meta, index, HWZRAM_DRV_ZERO);
		atomic64_inc(&hwzram->stats.zero_pages);
	}

	memcpy(meta->table[index].compressed_buffers, cmpl->buffers,
	       sizeof(phys_addr_t) * HWZRAM_MAX_BUFFERS_USED);

#ifdef CONFIG_MTK_ENG_BUILD
	meta->table[index].hash = cmpl->hash;
#endif
	if (cmpl->compr_status == HWZRAM_COMPRESSED)
		hwzram_set_obj_size(meta, index, compr_size);

	if (cmpl->compr_status == HWZRAM_COPIED)
		hwzram_set_obj_size(meta, index, UNCOMPRESSED_BLOCK_SIZE);

	bit_spin_unlock(HWZRAM_DRV_ACCESS, &meta->table[index].value);

	/* Update stats */
	atomic64_add(compr_size, &hwzram->stats.compr_data_size);
	atomic64_inc(&hwzram->stats.pages_stored);
	/* update_used_max(hwzram, cmpl->used_size); */

	if ((cmpl->compr_status == HWZRAM_COPIED) ||
	    (cmpl->compr_status == HWZRAM_COMPRESSED) ||
	    (cmpl->compr_status == HWZRAM_ZERO)) {
		if (cmpl->flags)
			bio_endio(bio);
	} else {
		bio_io_error(bio);
	}

	kfree(cmpl);
}

static int hwzram_bvec_write(struct hwzram *hwzram, struct bio_vec *bvec, u32 index,
			     int offset, struct bio *bio, bool last)
{
	int ret;
	struct hwzram_impl_completion *cmpl = kzalloc(sizeof(*cmpl),
						      GFP_ATOMIC);

	/* !!! make this a static allocation of fifo_size */
	if (!cmpl) {
		ret = -ENOMEM;
		goto out;
	}

	if (last)
		cmpl->flags = 1;
	cmpl->index = index;
	cmpl->compr_status = HWZRAM_IDLE;
	cmpl->hwzram = hwzram;
	cmpl->data = bio;
	cmpl->callback = hwzram_impl_compr_cb;
	ret = hwzram_impl_compress_page(hwzram->hwz, bvec->bv_page, cmpl);

	/* TBC - miss error handling */
	if (ret != 0) {
		pr_err("%s: error %d\n", __func__, ret);
		kfree(cmpl);
	}
out:
	return ret;
}

static int hwzram_bvec_rw(struct hwzram *hwzram, struct bio_vec *bvec, u32 index,
			  int offset, struct bio *bio, bool last)
{
	int ret;
	int rw = bio_data_dir(bio);

	if (rw == READ) {
		atomic64_inc(&hwzram->stats.num_reads);
		ret = hwzram_bvec_read(hwzram, bvec, index, offset, bio, last);
	} else {
		atomic64_inc(&hwzram->stats.num_writes);
		ret = hwzram_bvec_write(hwzram, bvec, index, offset, bio, last);
	}

	if (unlikely(ret)) {
		if (rw == READ)
			atomic64_inc(&hwzram->stats.failed_reads);
		else
			atomic64_inc(&hwzram->stats.failed_writes);
	}

	return ret;
}

/*
 * hwzram_bio_discard - handler on discard request
 * @index: physical block index in PAGE_SIZE units
 * @offset: byte offset within physical block
 */
static void hwzram_bio_discard(struct hwzram *hwzram, u32 index,
			     int offset, struct bio *bio)
{
	size_t n = bio->bi_iter.bi_size;
	struct hwzram_meta *meta = hwzram->meta;

	/*
	 * hwzram manages data in physical block size units. Because logical block
	 * size isn't identical with physical block size on some arch, we
	 * could get a discard request pointing to a specific offset within a
	 * certain physical block.  Although we can handle this request by
	 * reading that physiclal block and decompressing and partially zeroing
	 * and re-compressing and then re-storing it, this isn't reasonable
	 * because our intent with a discard request is to save memory.  So
	 * skipping this logical block is appropriate here.
	 */
	if (offset) {
		if (n <= (PAGE_SIZE - offset))
			return;

		n -= (PAGE_SIZE - offset);
		index++;
	}

	while (n >= PAGE_SIZE) {
		bit_spin_lock(HWZRAM_DRV_ACCESS, &meta->table[index].value);
		hwzram_free_page(hwzram, index);
		bit_spin_unlock(HWZRAM_DRV_ACCESS, &meta->table[index].value);
		atomic64_inc(&hwzram->stats.notify_free);
		index++;
		n -= PAGE_SIZE;
	}
}

static void hwzram_reset_device(struct hwzram *hwzram, bool reset_capacity)
{
	size_t index;
	struct hwzram_meta *meta;

	down_write(&hwzram->init_lock);

	hwzram->limit_pages = 0;

	if (!init_done(hwzram)) {
		up_write(&hwzram->init_lock);
		return;
	}

	meta = hwzram->meta;
	/* Free all pages that are still in this hwzram device */
	for (index = 0; index < hwzram->disksize >> PAGE_SHIFT; index++) {
		hwzram_impl_free_buffers(hwzram->hwz,
					 meta->table[index].compressed_buffers);
	}

	hwzram_meta_free(hwzram->meta);
	hwzram->meta = NULL;
	/* Reset stats */
	memset(&hwzram->stats, 0, sizeof(hwzram->stats));

	hwzram->disksize = 0;
	if (reset_capacity)
		set_capacity(hwzram->disk, 0);

	up_write(&hwzram->init_lock);

	/*
	 * Revalidate disk out of the init_lock to avoid lockdep splat.
	 * It's okay because disk's capacity is protected by init_lock
	 * so that revalidate_disk always sees up-to-date capacity.
	 */
	if (reset_capacity)
		revalidate_disk(hwzram->disk);
}

static ssize_t disksize_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
#define DEFAULT_DISKSIZE_PERC_RAM	(50)
#define SUPPOSED_TOTALRAM	(0x20000)       /* 512MB */

	u64 disksize;
	struct hwzram_meta *meta;
	struct hwzram *hwzram = dev_to_hwzram(dev);
	int err;

	disksize = memparse(buf, NULL);
	if (!disksize) {
		/* Give it a default disksize */
		disksize = DEFAULT_DISKSIZE_PERC_RAM * ((totalram_pages << PAGE_SHIFT) / 100);
		/* Promote the default disksize if totalram_pages is smaller */
		if (totalram_pages < SUPPOSED_TOTALRAM)
			disksize += (disksize >> 1);
	}

	disksize = PAGE_ALIGN(disksize);
	meta = hwzram_meta_alloc(disksize);
	if (!meta)
		return -ENOMEM;

	down_write(&hwzram->init_lock);
	if (init_done(hwzram)) {
		pr_info("Cannot change disksize for initialized device\n");
		err = -EBUSY;
		goto out_up_write;
	}

	hwzram->meta = meta;
	hwzram->disksize = disksize;
	set_capacity(hwzram->disk, hwzram->disksize >> SECTOR_SHIFT);
	up_write(&hwzram->init_lock);

	/*
	 * Revalidate disk out of the init_lock to avoid lockdep splat.
	 * It's okay because disk's capacity is protected by init_lock
	 * so that revalidate_disk always sees up-to-date capacity.
	 */
	revalidate_disk(hwzram->disk);

	return len;

out_up_write:
	up_write(&hwzram->init_lock);

	hwzram_meta_free(meta);
	return err;
}

static ssize_t reset_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	int ret;
	unsigned short do_reset;
	struct hwzram *hwzram;
	struct block_device *bdev;

	hwzram = dev_to_hwzram(dev);
	bdev = bdget_disk(hwzram->disk, 0);

	if (!bdev)
		return -ENOMEM;

	/* Do not reset an active device! */
	if (bdev->bd_holders) {
		ret = -EBUSY;
		goto out;
	}

	ret = kstrtou16(buf, 10, &do_reset);
	if (ret)
		goto out;

	if (!do_reset) {
		ret = -EINVAL;
		goto out;
	}

	/* Make sure all pending I/O is finished */
	fsync_bdev(bdev);
	bdput(bdev);

	hwzram_reset_device(hwzram, true);
	return len;

out:
	bdput(bdev);
	return ret;
}

static int __hwzram_make_request(struct hwzram *hwzram, struct bio *bio)
{
	int offset;
	u32 index;
	struct bio_vec bvec;
	struct bvec_iter iter;

	index = bio->bi_iter.bi_sector >> SECTORS_PER_PAGE_SHIFT;
	offset = (bio->bi_iter.bi_sector &
		  (SECTORS_PER_PAGE - 1)) << SECTOR_SHIFT;

	if (unlikely(bio->bi_rw & REQ_DISCARD)) {
		hwzram_bio_discard(hwzram, index, offset, bio);
		bio_endio(bio);
		return 0;
	}

	pr_devel("%s: bio bi_phys_segments %u bi_remaining = %u bi_vcnt = %u\n",
	       __func__, bio->bi_phys_segments, atomic_read(&bio->__bi_remaining),
	       bio->bi_vcnt);

	/* To avoid race condition between bio_for_each_segment & asynchronous bio_endio */
	bio_get(bio);

	bio_for_each_segment(bvec, bio, iter) {
		int segment = 0;
		bool last = false;
		int max_transfer_size = PAGE_SIZE - offset;

		/* !!! hack to deal with large bios */
		if (iter.bi_size == bvec.bv_len)
			last = true;

		pr_devel("%s: segment %d bvec.bv_len = %u iter.bi_sector = %lu iter.bi_size = %u iter.bi_idx = %u iter.bi_bvec_done = %u last = %u\n",
				__func__, segment, bvec.bv_len, (unsigned long) iter.bi_sector, iter.bi_size,
				iter.bi_idx, iter.bi_bvec_done, last);

		if (bvec.bv_len > max_transfer_size) {
			/*
			 * hwzram_bvec_rw() can only make operation on a single
			 * hwzram page. Split the bio vector.
			 */
			struct bio_vec bv;

			pr_info("%s: split bio vector case bvec.bv_len %d max %d\n",
				__func__, bvec.bv_len, max_transfer_size);

			bv.bv_page = bvec.bv_page;
			bv.bv_len = max_transfer_size;
			bv.bv_offset = bvec.bv_offset;

			if (hwzram_bvec_rw(hwzram, &bv, index, offset, bio, last) < 0)
				goto out;

			bv.bv_len = bvec.bv_len - max_transfer_size;
			bv.bv_offset += max_transfer_size;
			if (hwzram_bvec_rw(hwzram, &bv, index + 1, 0, bio, last) < 0)
				goto out;
		} else {
			if (hwzram_bvec_rw(hwzram, &bvec, index, offset, bio, last) < 0)
				goto out;
		}

		update_position(&index, &offset, &bvec);
		segment++;
	}

	bio_put(bio);
	return 0;

out:
	bio_put(bio);
	bio_io_error(bio);
	return 1;
}

/*
 * Handler function for all hwzram I/O requests.
 */
static blk_qc_t hwzram_make_request(struct request_queue *queue, struct bio *bio)
{
	struct hwzram *hwzram = queue->queuedata;

	down_read(&hwzram->init_lock);
	if (unlikely(!init_done(hwzram)))
		goto error;

	if (!valid_io_request(hwzram, bio)) {
		atomic64_inc(&hwzram->stats.invalid_io);
		goto error;
	}

	__hwzram_make_request(hwzram, bio);
	up_read(&hwzram->init_lock);

	return BLK_QC_T_NONE;

error:
	up_read(&hwzram->init_lock);
	bio_io_error(bio);
	return BLK_QC_T_NONE;
}

/* This may be called from interrupt context */
static void hwzram_slot_free_notify(struct block_device *bdev,
				unsigned long index)
{
	struct hwzram *hwzram;
	struct hwzram_meta *meta;

	hwzram = bdev->bd_disk->private_data;
	meta = hwzram->meta;

	bit_spin_lock(HWZRAM_DRV_ACCESS, &meta->table[index].value);
	hwzram_free_page(hwzram, index);
	bit_spin_unlock(HWZRAM_DRV_ACCESS, &meta->table[index].value);
	atomic64_inc(&hwzram->stats.notify_free);
}

static void hwzram_release(struct gendisk *disk, fmode_t mode)
{
	struct hwzram *hwzram = disk->private_data;

	atomic_dec(&hwzram->nr_opens);
}

static int hwzram_open(struct block_device *bdev, fmode_t mode)
{
	struct hwzram *hwzram = bdev->bd_disk->private_data;
	/*
	 * Chromium OS specific behavior:
	 * sys_swapon opens the device once to populate its swapinfo->swap_file
	 * and once when it claims the block device (blkdev_get).  By limiting
	 * the maximum number of opens to 2, we ensure there are no prior open
	 * references before swap is enabled.
	 * (Note, kzalloc ensures nr_opens starts at 0.)
	 */
	int open_count = atomic_inc_return(&hwzram->nr_opens);

	if (open_count > 2)
		goto busy;
	/*
	 * swapon(2) claims the block device after setup.  If a hwzram is claimed
	 * then open attempts are rejected. This is belt-and-suspenders as the
	 * the block device and swap_file will both hold open nr_opens until
	 * swapoff(2) is called.
	 */
	if (bdev->bd_holder != NULL)
		goto busy;
	return 0;
busy:
	pr_warn("open attempted while hwzram%d claimed (count: %d)\n",
			hwzram->disk->first_minor, open_count);
	atomic_dec(&hwzram->nr_opens);
	return -EBUSY;
}

static const struct block_device_operations hwzram_devops = {
	.open = hwzram_open,
	.release = hwzram_release,
	.swap_slot_free_notify = hwzram_slot_free_notify,
	.owner = THIS_MODULE
};

static DEVICE_ATTR(disksize, S_IRUGO | S_IWUSR,
		disksize_show, disksize_store);
static DEVICE_ATTR(initstate, S_IRUGO, initstate_show, NULL);
static DEVICE_ATTR(reset, S_IWUSR, NULL, reset_store);
static DEVICE_ATTR(orig_data_size, S_IRUGO, orig_data_size_show, NULL);
static DEVICE_ATTR(mem_used_total, S_IRUGO, mem_used_total_show, NULL);
static DEVICE_ATTR(mem_limit, S_IRUGO | S_IWUSR, mem_limit_show,
		mem_limit_store);
static DEVICE_ATTR(mem_used_max, S_IRUGO | S_IWUSR, mem_used_max_show,
		mem_used_max_store);

HWZRAM_ATTR_RO(num_reads);
HWZRAM_ATTR_RO(num_writes);
HWZRAM_ATTR_RO(failed_reads);
HWZRAM_ATTR_RO(failed_writes);
HWZRAM_ATTR_RO(invalid_io);
HWZRAM_ATTR_RO(notify_free);
HWZRAM_ATTR_RO(zero_pages);
HWZRAM_ATTR_RO(compr_data_size);

static struct attribute *hwzram_disk_attrs[] = {
	&dev_attr_disksize.attr,
	&dev_attr_initstate.attr,
	&dev_attr_reset.attr,
	&dev_attr_num_reads.attr,
	&dev_attr_num_writes.attr,
	&dev_attr_failed_reads.attr,
	&dev_attr_failed_writes.attr,
	&dev_attr_invalid_io.attr,
	&dev_attr_notify_free.attr,
	&dev_attr_zero_pages.attr,
	&dev_attr_orig_data_size.attr,
	&dev_attr_compr_data_size.attr,
	&dev_attr_mem_used_total.attr,
	&dev_attr_mem_limit.attr,
	&dev_attr_mem_used_max.attr,
	NULL,
};

static struct attribute_group hwzram_disk_attr_group = {
	.attrs = hwzram_disk_attrs,
};

/* (in bytes) - lockless */
unsigned long hwzram_mem_used_total(void)
{
	struct hwzram *hwzram = hwzram_devices;
	unsigned long val = 0;

	if (!hwzram)
		return 0;

	if (init_done(hwzram))
		val = hwzram_impl_get_total_used(hwzram->hwz);

	return val;
}

#ifdef CONFIG_PROC_FS
void hwzraminfo_proc_show(struct seq_file *m, void *v)
{
	struct hwzram *hwzram = hwzram_devices;

	if (!hwzram)
		return;

	if (num_devices == 1 && init_done(hwzram)) {
#define P2K(x) (((unsigned long)x) << (PAGE_SHIFT - 10))
#define B2K(x) (((unsigned long)x) >> (10))
		seq_printf(m,
				"DiskSize:       %8lu kB\n"
				"OrigSize:       %8lu kB\n"
				"ComprSize:      %8lu kB\n"
				"MemUsed:        %8lu kB\n"
				"ZeroPage:       %8lu kB\n"
				"NotifyFree:     %8lu kB\n"
				"FailReads:      %8lu kB\n"
				"FailWrites:     %8lu kB\n"
				"NumReads:       %8lu kB\n"
				"NumWrites:      %8lu kB\n"
				"InvalidIO:      %8lu kB\n"
				"MaxUsedPages:   %8lu kB\n"
				,
				B2K(hwzram->disksize),
				P2K(atomic64_read(&hwzram->stats.pages_stored)),
				B2K(atomic64_read(&hwzram->stats.compr_data_size)),
				B2K(hwzram_impl_get_total_used(hwzram->hwz)),
				P2K(atomic64_read(&hwzram->stats.zero_pages)),
				P2K(atomic64_read(&hwzram->stats.notify_free)),
				P2K(atomic64_read(&hwzram->stats.failed_reads)),
				P2K(atomic64_read(&hwzram->stats.failed_writes)),
				P2K(atomic64_read(&hwzram->stats.num_reads)),
				P2K(atomic64_read(&hwzram->stats.num_writes)),
				P2K(atomic64_read(&hwzram->stats.invalid_io)),
				P2K(atomic_long_read(&hwzram->stats.max_used_pages)));
#undef P2K
#undef B2K
	}
}
#endif

static int create_device(struct hwzram *hwzram, int device_id,
			 struct hwzram_impl *hwz)
{
	int ret = -ENOMEM;

	hwzram->hwz = hwz;
	init_rwsem(&hwzram->init_lock);

	hwzram->queue = blk_alloc_queue(GFP_KERNEL);
	if (!hwzram->queue) {
		pr_err("Error allocating disk queue for device %d\n",
			device_id);
		goto out;
	}

	blk_queue_make_request(hwzram->queue, hwzram_make_request);
	hwzram->queue->queuedata = hwzram;

	 /* gendisk structure */
	hwzram->disk = alloc_disk(1);
	if (!hwzram->disk) {
		pr_warn("Error allocating disk structure for device %d\n",
			device_id);
		goto out_free_queue;
	}

	hwzram->disk->major = hwzram_major;
	hwzram->disk->first_minor = device_id;
	hwzram->disk->fops = &hwzram_devops;
	hwzram->disk->queue = hwzram->queue;
	hwzram->disk->private_data = hwzram;
	snprintf(hwzram->disk->disk_name, 16, "hwzram%d", device_id);

	/* Actual capacity set using syfs (/sys/block/hwzram<id>/disksize */
	set_capacity(hwzram->disk, 0);
	/* hwzram devices sort of resembles non-rotational disks */
	queue_flag_set_unlocked(QUEUE_FLAG_NONROT, hwzram->disk->queue);
	queue_flag_clear_unlocked(QUEUE_FLAG_ADD_RANDOM, hwzram->disk->queue);
	/*
	 * To ensure that we always get PAGE_SIZE aligned
	 * and n*PAGE_SIZED sized I/O requests.
	 */
	blk_queue_physical_block_size(hwzram->disk->queue, PAGE_SIZE);
	blk_queue_logical_block_size(hwzram->disk->queue,
					HWZRAM_LOGICAL_BLOCK_SIZE);
	blk_queue_io_min(hwzram->disk->queue, PAGE_SIZE);
	blk_queue_io_opt(hwzram->disk->queue, PAGE_SIZE);
	hwzram->disk->queue->limits.discard_granularity = PAGE_SIZE;
	/* hwzram->disk->queue->limits.max_discard_sectors = UINT_MAX; */
	blk_queue_max_discard_sectors(hwzram->disk->queue, UINT_MAX);
	/*
	 * hwzram_bio_discard() will clear all logical blocks if logical block
	 * size is identical with physical block size(PAGE_SIZE). But if it is
	 * different, we will skip discarding some parts of logical blocks in
	 * the part of the request range which isn't aligned to physical block
	 * size.  So we can't ensure that all discarded logical blocks are
	 * zeroed.
	 */
	if (HWZRAM_LOGICAL_BLOCK_SIZE == PAGE_SIZE)
		hwzram->disk->queue->limits.discard_zeroes_data = 1;
	else
		hwzram->disk->queue->limits.discard_zeroes_data = 0;
	queue_flag_set_unlocked(QUEUE_FLAG_DISCARD, hwzram->disk->queue);

	add_disk(hwzram->disk);

	ret = sysfs_create_group(&disk_to_dev(hwzram->disk)->kobj,
				&hwzram_disk_attr_group);
	if (ret < 0) {
		pr_warn("Error creating sysfs group");
		goto out_free_disk;
	}

	hwzram->meta = NULL;

	return 0;

out_free_disk:
	del_gendisk(hwzram->disk);
	put_disk(hwzram->disk);
out_free_queue:
	blk_cleanup_queue(hwzram->queue);
out:
	return ret;
}

static void destroy_device(struct hwzram *hwzram)
{

	if (!hwzram)
		return;

	if (hwzram->disk) {
		sysfs_remove_group(&disk_to_dev(hwzram->disk)->kobj,
				   &hwzram_disk_attr_group);

		del_gendisk(hwzram->disk);
		put_disk(hwzram->disk);
	}
	if (hwzram->queue)
		blk_cleanup_queue(hwzram->queue);

	if (hwzram->hwz)
		hwzram_return_impl(hwzram->hwz);
}


int hwzram_create_devices(int num_devices)
{
	int ret, dev_id, created = 0;

	pr_info("%s: num_devices %d\n", __func__, num_devices);
	if (!hwzram_devices) {
		pr_info("%s: hwzram_devices not yet initialized!\n", __func__);
		return 0;
	}
	for (dev_id = 0; dev_id < max_num_devices; dev_id++) {
		struct hwzram_impl *hwz;

		if (hwzram_devices[dev_id].used)
			continue;

		hwz = hwzram_get_impl();
		if (hwz) {
			ret = create_device(&hwzram_devices[dev_id], dev_id,
					    hwz);
			if (ret) {
				hwzram_return_impl(hwz);
				continue;
			}
			hwzram_devices[dev_id].used = true;
			created++;
			if (created == num_devices)
				return created;
		}
	}

	return created;
}

static int __init hwzram_init(void)
{
	int ret, created = 0;

	if (num_devices > max_num_devices) {
		pr_warn("Invalid value for num_devices: %u\n",
				num_devices);
		ret = -EINVAL;
		goto out;
	}

	hwzram_major = register_blkdev(0, "hwzram");
	if (hwzram_major <= 0) {
		pr_warn("Unable to get major number\n");
		ret = -EBUSY;
		goto out;
	}

	/* Allocate the device array and initialize each one */
	hwzram_devices = kcalloc(num_devices, sizeof(struct hwzram), GFP_KERNEL);
	if (!hwzram_devices) {
		ret = -ENOMEM;
		goto unregister;
	}

	created = hwzram_create_devices(num_devices);
	pr_info("Created %u device(s) ...\n", created);

	return 0;

unregister:
	unregister_blkdev(hwzram_major, "hwzram");
out:
	return ret;
}

static void __exit hwzram_exit(void)
{
	int i;
	struct hwzram *hwzram;

	for (i = 0; i < num_devices; i++) {
		hwzram = &hwzram_devices[i];
		/*
		 * Shouldn't access hwzram->disk after destroy_device
		 * because destroy_device already released hwzram->disk.
		 */
		hwzram_reset_device(hwzram, false);
		destroy_device(hwzram);
	}

	unregister_blkdev(hwzram_major, "hwzram");

	kfree(hwzram_devices);
	pr_debug("Cleanup done!\n");
}

module_init(hwzram_init);
module_exit(hwzram_exit);

module_param(num_devices, uint, 0);
MODULE_PARM_DESC(num_devices, "Number of hwzram devices");

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Sonny Rao <sonnyrao@chromium.org>");
MODULE_AUTHOR("Mediatek");
MODULE_DESCRIPTION("Hardware Compressed RAM Block Device");

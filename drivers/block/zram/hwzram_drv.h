/*
 *  Hardware Compressed RAM block device
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
 *  Based on Compressed RAM block device (zram)
 *
 *  Copyright (C) 2008, 2009, 2010  Nitin Gupta
 *                2012, 2013 Minchan Kim
 *
 *  This code is released using a dual license strategy: BSD/GPL
 *  You can choose the licence that better fits your requirements.
 *
 *  Released under the terms of 3-clause BSD License
 *  Released under the terms of GNU General Public License Version 2.0
 *
 */

#ifndef _HWZRAM_DRV_H_
#define _HWZRAM_DRV_H_

#include <linux/spinlock.h>
#include <linux/atomic.h>
#include <linux/zsmalloc.h>

#include "hwzram_impl.h"

/*
 * Some arbitrary value. This is just to catch
 * invalid value for num_devices module parameter.
 */
static const unsigned max_num_devices = 32;

#define SECTOR_SHIFT		9
#define SECTORS_PER_PAGE_SHIFT	(PAGE_SHIFT - SECTOR_SHIFT)
#define SECTORS_PER_PAGE	(1 << SECTORS_PER_PAGE_SHIFT)
#define HWZRAM_LOGICAL_BLOCK_SHIFT 12
#define HWZRAM_LOGICAL_BLOCK_SIZE	(1 << HWZRAM_LOGICAL_BLOCK_SHIFT)
#define HWZRAM_SECTOR_PER_LOGICAL_BLOCK	\
	(1 << (HWZRAM_LOGICAL_BLOCK_SHIFT - SECTOR_SHIFT))


/*
 * The lower ZRAM_FLAG_SHIFT bits of table.value is for
 * object size (excluding header), the higher bits is for
 * zram_pageflags.
 *
 * zram is mainly used for memory efficiency so we want to keep memory
 * footprint small so we can squeeze size and flags into a field.
 * The lower ZRAM_FLAG_SHIFT bits is for object size (excluding header),
 * the higher bits is for zram_pageflags.
 */
#define HWZRAM_FLAG_SHIFT 24

/* Flags for zram pages (table[page_no].value) */
enum hwzram_pageflags {
	/* Page consists entirely of zeros */
	HWZRAM_DRV_ZERO = HWZRAM_FLAG_SHIFT + 1,
	HWZRAM_DRV_ACCESS,	/* page in now accessed */

	__NR_HWZRAM_PAGEFLAGS,
};

/*-- Data structures */

/* Allocated for each disk page */
struct hwzram_table_entry {
	unsigned long value;
	phys_addr_t compressed_buffers[HWZRAM_MAX_BUFFERS_USED];
#ifdef CONFIG_MTK_ENG_BUILD
	u32 hash;
#endif
};

struct hwzram_stats {
	atomic64_t compr_data_size;	/* compressed size of pages stored */
	atomic64_t num_reads;	/* failed + successful */
	atomic64_t num_writes;	/* --do-- */
	atomic64_t failed_reads;	/* can happen when memory is too low */
	atomic64_t failed_writes;	/* can happen when memory is too low */
	atomic64_t invalid_io;	/* non-page-aligned I/O requests */
	atomic64_t notify_free;	/* no. of swap slot free notifications */
	atomic64_t zero_pages;		/* no. of zero filled pages */
	atomic64_t pages_stored;	/* no. of pages currently stored */
	atomic_long_t max_used_pages;	/* no. of maximum pages stored */
};

struct hwzram_meta {
	struct hwzram_table_entry *table;
};

struct hwzram {
	bool used;
	struct hwzram_meta *meta;
	struct request_queue *queue;
	struct gendisk *disk;
	struct hwzram_impl *hwz;

	/* Prevent concurrent execution of device init, reset and R/W request */
	struct rw_semaphore init_lock;
	/*
	 * This is the limit on amount of *uncompressed* worth of data
	 * we can store in a disk.
	 */
	u64 disksize;	/* bytes */

	atomic_t nr_opens;	/* number of active file handles */
	struct hwzram_stats stats;
	/*
	 * the number of pages zram can consume for storing compressed data
	 */
	unsigned long limit_pages;

};
#endif /* _HWZRAM_DRV_H */

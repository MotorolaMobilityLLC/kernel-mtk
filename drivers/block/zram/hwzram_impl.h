/*
 *  Hardware Compressed RAM offload driver
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
 * Sonny Rao <sonnyrao@chromium.org>
 */
#ifndef _HWZRAM_IMPL_H_
#define _HWZRAM_IMPL_H_

#include <linux/atomic.h>
#include <linux/completion.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include "hwzram.h"


/*
 * Why do we have this "impl" thing?
 *
 * Its purpose is to separate out the code which interacts with the
 * hardware from the layers above it -- i.e. either zram (block
 * device) or zswap (front swap) or anything else.  This code also
 * handles memory allocation, so layers above might need to call into
 * this code to free up memory that is no longer used.
 */

struct hwzram_impl_stats {
	/* these are all protected by the lock on the fifo */
	uint64_t compr_total_commands;
	uint64_t fifo_full_stalls;
	/* uint64_t compr_status_compressed; */
	/* uint64_t compr_status_copied; */
	/* uint64_t compr_status_zero; */
	/* uint64_t compr_status_abort; */
	/* uint64_t compr_status_error; */
	uint64_t compr_fifo_resets;

	/* decompress is atomic to avoid doing things under a lock */
	atomic64_t total_decompr_commands;
};

/* !!! hacky memory allocator, re-uses slabs for now */
struct hwzram_impl_allocator {
	struct kmem_cache *size_allocator[ZRAM_NUM_OF_POSSIBLE_SIZES];
	atomic64_t size_count[ZRAM_NUM_OF_POSSIBLE_SIZES];
};

struct hwzram_impl_completion {
	u32 index;
	void *hwzram;
	void *data;
	uint32_t flags;

	void (*callback)(struct hwzram_impl_completion *);

	/* shared by compression and decompression */
	uint16_t compr_size;

	union {
		/* used for returning status of a compression */
		enum desc_status compr_status;
		/* used for returning status of a decompression */
		enum decompr_status decompr_status;
	};

	union {
		/* a copy of the source dma address, used by type 1 descriptors */
		dma_addr_t src_copy;
		/* copy of the destination dma address */
		dma_addr_t daddr;
	};

	union {
		/*
		 * These are the encoded addresses of the buffers used during
		 * compression. The encoding includes the size.
		 */
		phys_addr_t buffers[HWZRAM_MAX_BUFFERS_USED];

		/* copy of dma mapped source buffers */
		dma_addr_t dma_bufs[HWZRAM_MAX_BUFFERS_USED];
	};
#ifdef CONFIG_MTK_ENG_BUILD
	u32 hash;
#endif
};

#define HWZRAM_IMPL_MAX_NAME 8

enum platform_ops {
	/* compression */
	COMP_ENABLE,
	COMP_DISABLE,
	/* decompression */
	DECOMP_ENABLE,
	DECOMP_DISABLE,
};

struct hwzram_impl;
typedef void hwzram_impl_ops(struct hwzram_impl *hwz);

/* HW related controls */
typedef void hwzram_clock_control(enum platform_ops ops);

struct hwzram_impl {
	char name[HWZRAM_IMPL_MAX_NAME];
	unsigned int id;
	struct list_head hwzram_list;

	/* hardware characteristics */

	/* is the descriptor fifo fixed (in SRAM) or not (cacheable) */
	bool fifo_fixed;

	/* 4-byte vs 8-byte register access */
	bool full_width_reg_access;

	/*
	 * which descriptor type does this implementation use
	 * can be type 0 (64-byte) or type 1 (32-byte)
	 */
	uint8_t descriptor_type;
	uint8_t descriptor_size;

	/*
	 * implementations can specify how many buffers are used to store
	 * compressed data
	 */
	uint8_t max_buffer_count;

	/* need to flush out writes */
	bool need_cache_flush;

	/* need to invalidate prior to reads */
	bool need_cache_invalidate;

	/* how many decompression command sets are implemented */
	unsigned int decompr_cmd_count;

	/* relating to the fifo and masks used to do related calculations */
	uint16_t fifo_size;
	uint16_t fifo_index_mask;
	uint16_t fifo_color_mask;

	uint16_t write_index;

	/* cached copy of HW complete index */
	uint16_t complete_index;

	__iomem uint8_t *regs;

	/* in-memory allocated location (not aligned) for cacheable fifo */
	void *fifo_alloc;

	/* 64B aligned compression command fifo of either type 0 or type 1 */
	void *fifo;

	/* if needed, fifo pages mapped for dma */
	dma_addr_t fifo_dma_addr;
	struct dma_attrs fifo_dma_attrs;

	spinlock_t fifo_lock;

	/* Array of completions to keep track of each ongoing compression */
	struct hwzram_impl_completion **completions;

	/* cache of free buffers, can sleep if needed while refilling  */
	struct mutex cached_bufs_lock;
	phys_addr_t cached_bufs[ZRAM_NUM_OF_FREE_BLOCKS];

	/* array of atomic variables to keep track of decompression sets*/
	atomic_t *decompr_cmd_used;

	/* !!! use a better constant for the max here */
	struct hwzram_impl_completion *decompr_completions[16];

	/* parent device */
	struct device *dev;

	struct hwzram_impl_allocator allocator;
	struct hwzram_impl_stats stats;

	/* !!! single shared interrupt, need to add more interrupts */
	int irq;

	/* no interrupts, need to use a timer */
#define HWZRAM_TIMER_DELAY 100	/* 1 ~ 100 */
	bool need_timer;
	struct timer_list timer;

	/* HW clock control */
	hwzram_clock_control *hwctrl;
};

/* Platform specific initialization flow */
typedef void hwzram_platform_initcall(struct hwzram_impl *hwz, unsigned int vendor,
					unsigned int device);

/* find an already probed hwzram instance */
struct hwzram_impl *hwzram_get_impl(void);

/* return an hzram instance */
void hwzram_return_impl(struct hwzram_impl *hwz);

/*
 * initialize hardware block - mostly meant to be used by hardware probe
 * functions
 */
struct hwzram_impl *hwzram_impl_init(struct device *dev, uint16_t fifo_size,
				     phys_addr_t regs, int irq,
				     hwzram_platform_initcall platform_init,
				     hwzram_clock_control hwctrl);
/* tear down hardware block */
void hwzram_impl_destroy(struct hwzram_impl *hwz);

/* actions at suspend */
void hwzram_impl_suspend(struct hwzram_impl *hwz);

/* actions at resume */
void hwzram_impl_resume(struct hwzram_impl *hwz);

/*
 * start a compression, returns non-zero if fifo is full
 *
 * the completion argument includes a callback which is called when
 * the compression is completed and returns the size, status, and
 * the memory used to store the compressed data.
 */
int hwzram_impl_compress_page(struct hwzram_impl *hwz, struct page *page,
			      struct hwzram_impl_completion *cmpl);

int hwzram_impl_decompress_page(struct hwzram_impl *hwz,
				struct page *page,
				struct hwzram_impl_completion *cmpl);

int hwzram_impl_decompress_page_atomic(struct hwzram_impl *hwz,
				       struct page *page,
				       struct hwzram_impl_completion *cmpl);

int hwzram_impl_copy_buf(struct hwzram_impl *hwz, struct page *page,
			 phys_addr_t *buf);

/*
 * free a set of buffers which were returned through a compression
 * command completion
 */
void hwzram_impl_free_buffers(struct hwzram_impl *hwz, phys_addr_t *bufs);

uint64_t hwzram_impl_get_total_used(struct hwzram_impl *hwz);

#endif /* _HWZRAM_IMPL_H_ */

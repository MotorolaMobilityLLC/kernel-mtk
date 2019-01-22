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
 *  Modifications from MediaTek Inc.:
 *  1. Provide a vendor directory and callback for private initialization.
 *  2. Change vendor ids for Google and MTK.
 *  3. Compression fifo needs to be cleared to 0 before use.
 *  4. Change the register base for decompression.
 *  5. Handle HWZRAM_ABORT case.
 *  6. Set decompression command compress buffer to 0 to stand for no buffer.
 *  7. Update refill_cached_bufs.
 *  8. Add GFP_NOIO to avoid deadlock with reclaim path.
 *  9. Use polling in decompression flow for quick response.
 *
 * It is based on Hardware Compressed RAM block device (see below)
 *
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
 *  Sonny Rao <sonnyrao@chromium.org>
 *
 */

#ifdef CONFIG_HWZRAM_DEBUG
#define DEBUG
#endif

#include <asm/page.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/idr.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/highmem.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/atomic.h>
#include <asm/cacheflush.h>
#include <asm/irqflags.h>
#include <asm/page.h>
#include <asm/system_misc.h>
#include "hwzram_impl.h"

/* global ID number for zram devices */
static DEFINE_IDA(hwzram_ida);

/* list of all unclaimed hwzram devices */
static LIST_HEAD(hwzram_list);
static DEFINE_SPINLOCK(hwzram_list_lock);

static bool hwzram_force_non_coherent = true;
static bool hwzram_force_timer; /* default: false */
static bool hwzram_force_no_copy_enable = true;

/*
 * TODO:
 *
 * use Decompression result write
 * support hash option
 * support client specified buffers or sizes
 * handle errors properly
 */

static inline bool hwzram_non_coherent(struct hwzram_impl *hwz)
{
	if (hwzram_force_non_coherent ||
	    hwz->need_cache_flush ||
	    hwz->need_cache_invalidate)
		return true;
	return false;
}

static void *hwzram_alloc_size_noncoherent(struct hwzram_impl *hwz,
					uint16_t size, gfp_t gfp_flags)
{
	unsigned int index = ffs(size) - 1 - ZRAM_MIN_SIZE_SHIFT;
	struct kmem_cache *cache = hwz->allocator.size_allocator[index];
	void *virt;

	pr_devel("[%s] %s %s size %u (idx %u) cache %p\n", current->comm,
		 hwz->name, __func__, size, index, cache);

	virt = kmem_cache_alloc(cache, gfp_flags);
	if (virt) {
		atomic64_inc(&hwz->allocator.size_count[index]);
		return virt;
	}

	pr_devel("[%s] %s %s size %u failed\n", current->comm,
			__func__, hwz->name, size);
	return 0;
}

/* !!! hack allocator for now TODO(sonnyrao) fix this */
static dma_addr_t hwzram_alloc_size(struct hwzram_impl *hwz, uint16_t size,
				    gfp_t gfp_flags)
{
	unsigned int index = ffs(size) - 1 - ZRAM_MIN_SIZE_SHIFT;
	struct kmem_cache *cache = hwz->allocator.size_allocator[index];
	void *virt;

	pr_devel("[%s] %s %s size %u (idx %u) cache %p\n", current->comm,
		 hwz->name, __func__, size, index, cache);

	virt = kmem_cache_alloc(cache, gfp_flags);
	if (virt) {
		atomic64_inc(&hwz->allocator.size_count[index]);
		if (hwzram_non_coherent(hwz)) {
			dma_addr_t daddr = dma_map_single(hwz->dev,
							  virt,
							  size,
							  DMA_FROM_DEVICE);
			if (dma_mapping_error(hwz->dev, daddr)) {
				pr_err("%s: error dma mapping %p\n",
				       hwz->name, virt);
				kmem_cache_free(cache, virt);
				return 0;
			}

			return daddr;
		} else {
			return virt_to_phys(virt);
		}
	} else {
		pr_devel("[%s] %s %s size %u failed\n", current->comm,
			 __func__, hwz->name, size);
	}

	return 0;
}

void hwzram_impl_free_buffers(struct hwzram_impl *hwz, phys_addr_t *bufs)
{
	int i;
	phys_addr_t addr;
	uint16_t size;
	int index;
	struct kmem_cache *cache;

	for (i = 0; i < HWZRAM_MAX_BUFFERS_USED; i++) {
		addr = bufs[i];
		if (!addr)
			continue;
		size = ENCODED_ADDR_TO_SIZE(addr);
		addr = ENCODED_ADDR_TO_PHYS(addr);
		WARN_ON(!size);
		index = ffs(size) - 1 - ZRAM_MIN_SIZE_SHIFT;
		cache = hwz->allocator.size_allocator[index];

		pr_devel("%s: freeing encoded %pa size %u addr %pa cache %p\n",
			__func__, &bufs[i], size, &addr, cache);

		kmem_cache_free(cache, phys_to_virt(addr));
		atomic64_dec(&hwz->allocator.size_count[index]);
	}
}
EXPORT_SYMBOL(hwzram_impl_free_buffers);

static void hwzram_impl_free_fifos(struct hwzram_impl *hwz, uint64_t *bufs)
{
	int i;
	phys_addr_t addr;
	uint16_t size;
	int index;
	struct kmem_cache *cache;

	for (i = 0; i < ZRAM_NUM_OF_FREE_BLOCKS; i++) {
		addr = bufs[i];
		if (!addr)
			continue;
		size = ENCODED_ADDR_TO_SIZE(addr);
		addr = ENCODED_ADDR_TO_PHYS(addr);
		WARN_ON(!size);
		index = ffs(size) - 1 - ZRAM_MIN_SIZE_SHIFT;
		cache = hwz->allocator.size_allocator[index];

		pr_devel("%s: freeing encoded %pa size %u addr %pa cache %p\n",
			__func__, &bufs[i], size, &addr, cache);

		/* convert to phys */
		if (hwzram_non_coherent(hwz)) {
			dma_addr_t daddr = addr;

			addr = dma_to_phys(hwz->dev, daddr);
			dma_unmap_single(hwz->dev, daddr, size,
					DMA_FROM_DEVICE);
		}

		kmem_cache_free(cache, phys_to_virt(addr));
		atomic64_dec(&hwz->allocator.size_count[index]);
	}
}

/* TODO(sonnyrao) when using non-hacky allocator need to count fragmentation */
uint64_t hwzram_impl_get_total_used(struct hwzram_impl *hwz)
{
	int i;
	uint64_t tmp, total = 0;

	for (i = 0; i < ZRAM_NUM_OF_POSSIBLE_SIZES; i++) {
		tmp = atomic64_read(&hwz->allocator.size_count[i]);
		pr_devel("%s: size(%u) count 0x%llx total 0x%llx\n",
			 __func__,
			 (1 << (i + ZRAM_MIN_SIZE_SHIFT)),
			 tmp,
			 (tmp << (i + ZRAM_MIN_SIZE_SHIFT)));
		total += tmp << (i + ZRAM_MIN_SIZE_SHIFT);
	}
	pr_devel("%s: returning 0x%llx\n", __func__, total);
	return total;
}
EXPORT_SYMBOL(hwzram_impl_get_total_used);

static void hwzram_allocator_destroy(struct hwzram_impl_allocator *allocator)
{
	int j;

	for (j = 0; j < ZRAM_NUM_OF_POSSIBLE_SIZES; j++)
		kmem_cache_destroy(allocator->size_allocator[j]);
}

static int hwzram_allocator_init(struct hwzram_impl_allocator *allocator,
				 unsigned int id)
{
	int i;

	memset(allocator, 0, sizeof(*allocator));

	/* one slab for each possible size 64 through 4096 */
	for (i = 0; i < ZRAM_NUM_OF_POSSIBLE_SIZES; i++) {
		char name[32];
		unsigned int size = 1 << (i + ZRAM_MIN_SIZE_SHIFT);

		snprintf(name, 32, "hwzram-%u-size-%u", id, size);
		allocator->size_allocator[i] = kmem_cache_create(name,
								 size, size,
							      SLAB_CACHE_DMA |
							      SLAB_HWCACHE_ALIGN,
							      NULL);
		if (!allocator->size_allocator[i]) {
			pr_err("unable to create alloctor for size %u\n", size);
			hwzram_allocator_destroy(allocator);
			return -1;

		}
		atomic64_set(&allocator->size_count[i], 0);
		pr_devel("created slab %s at %p\n", name, allocator->size_allocator[i]);
	}

	return 0;
}

static void hwzram_write_register(struct hwzram_impl *hwz, unsigned int offset,
				  uint64_t val)
{
	if (!hwz->full_width_reg_access) {
		volatile uint32_t *lo = (uint32_t *)&hwz->regs[offset];
		volatile uint32_t *hi = (uint32_t *)&hwz->regs[offset +
						     sizeof(uint32_t)];
		writel(val >> 32ULL, hi);
		writel(val & 0xFFFFFFFFULL, lo);
	} else {
#if defined(writeq)
		volatile uint64_t *reg = (uint64_t *)&hwz->regs[offset];

		writeq(val, reg);
#endif
	}
}

static uint64_t hwzram_read_register(struct hwzram_impl *hwz,
				     unsigned int offset)
{
	uint64_t ret = 0;

	if (!hwz->full_width_reg_access) {
		volatile uint32_t *lo = (uint32_t *)(hwz->regs + offset);
		volatile uint32_t *hi = (uint32_t *)(hwz->regs + offset +
					   sizeof(uint32_t));
		ret = (uint64_t)readl(hi) << 32ULL;
		ret |= (uint64_t)readl(lo);
	} else {
#if defined(readq)
		ret = readq(hwz->regs + offset);
#endif
	}

	return ret;
}

static void hwzram_impl_dump_regs(struct hwzram_impl *hwz)
{
	unsigned int offset = 0;

	pr_info("%s: dump_regs: global\n", hwz->name);
	for (offset = ZRAM_HWID; offset <= ZRAM_ERR_MSK; offset += 8)
		pr_info("%s: 0x%03X: 0x%016llX\n", hwz->name, offset,
			hwzram_read_register(hwz, offset));

	pr_info("%s: dump_regs: global (decompression)\n", hwz->name);
	for (offset = ZRAM_HWID; offset <= ZRAM_ERR_MSK; offset += 8)
		pr_info("%s: 0x%03X: 0x%016llX\n", hwz->name, offset,
			hwzram_read_register(hwz, ZRAM_DECOMP_OFFSET + offset));

	pr_info("%s: dump_regs: compression\n", hwz->name);
	for (offset = ZRAM_CDESC_LOC; offset <= ZRAM_CINTERP_CTRL; offset += 8)
		pr_info("%s: 0x%03X: 0x%016llX\n", hwz->name, offset,
			hwzram_read_register(hwz, offset));

	pr_info("%s: dump_regs: decompression\n", hwz->name);
	for (offset = ZRAM_CDESC_LOC; offset <= ZRAM_CINTERP_CTRL; offset += 8)
		pr_info("%s: 0x%03X: 0x%016llX\n", hwz->name, offset,
			hwzram_read_register(hwz, ZRAM_DECOMP_OFFSET + offset));

	pr_info("%s: dump_regs: vendor\n", hwz->name);
	for (offset = ZRAM_BUSCFG; offset <= 0x118; offset += 8)
		pr_info("%s: 0x%03X: 0x%016llX\n", hwz->name, offset,
			hwzram_read_register(hwz, offset));

	pr_info("%s: dump_regs: vendor (decompression)\n", hwz->name);
	for (offset = ZRAM_BUSCFG; offset <= 0x118; offset += 8)
		pr_info("%s: 0x%03X: 0x%016llX\n", hwz->name, offset,
			hwzram_read_register(hwz, ZRAM_DECOMP_OFFSET + offset));
}

/* for debug purpose */
static void hwzram_impl_dump_regs_for_dec(struct hwzram_impl *hwz)
{
	unsigned int offset;

	/* DEC_STATUS */
	pr_alert("%s: 0x1904: 0x%016llX\n", hwz->name, hwzram_read_register(hwz, 0x1904));

	/* DEC_DBG_ERROR_STATUS */
	pr_alert("%s: 0x1A18: 0x%016llX\n", hwz->name, hwzram_read_register(hwz, 0x1A18));

	/* Decompression registers */
	for (offset = 0x800; offset <= 0x830; offset += 8)
		pr_info("%s: 0x%03X: 0x%016llX\n", hwz->name, offset,
			hwzram_read_register(hwz, ZRAM_DECOMP_OFFSET + offset));
}

/* restore dec */
static void hwzram_impl_restore_dec(struct hwzram_impl *hwz)
{
	unsigned int count = 0;

	/* clear error bit */
	hwzram_write_register(hwz, ZRAM_DECOMP_OFFSET + ZRAM_INTRP_STS_ERROR, 0x1);
	wmb();

	/* restore dec: DEC_GCTRL */
	hwzram_write_register(hwz, ZRAM_DECOMP_OFFSET + ZRAM_GCTRL, 0x1);
	while (count < 100 &&
	       hwzram_read_register(hwz, ZRAM_DECOMP_OFFSET + ZRAM_GCTRL)) {
		udelay(20);
		count++;
	}
	if (count == 100)
		pr_alert("%s: failed to restore dec\n", __func__);
}

static void convert_desc1(struct compr_desc_0 *dest,
			  struct compr_desc_1 *src,
			  dma_addr_t src_copy)
{
	int i;

	dest->hash = src->u1.hash;
	dest->compr_len = src->u2.status.compressed_size;
	dest->buf_sel = src->u2.status.buf_sel;
	/*
	 * for type 1 descriptors the source gets clobbered
	 * so we have to use a copy from the completion struct
	 */
	dest->u1.src_addr = src_copy;
	dest->u1.s1.status = src->u2.status.status;
	dest->u1.s1.intr_request = src->u2.status.interrupt_request;


	for (i = 0 ; i < ZRAM_NUM_OF_FREE_BLOCKS; i++)
		dest->dst_addr[i] = src->dst_addr[i] << 5;
}

static void hwzram_impl_fifo_cleanup(struct hwzram_impl *hwz)
{
	int i;
	void *p;
	struct compr_desc_0 *desc;
	struct compr_desc_0 tmp;
	struct compr_desc_1 *desc1 = NULL;

	for (i = 0 ; i < hwz->fifo_size; i++) {
		p = hwz->fifo + (i * hwz->descriptor_size);
		if (hwz->descriptor_type) {
			/* convert back to desc0 so addresses aren't shifted */
			convert_desc1(&tmp, p, 0);
			desc = &tmp;
			desc1 = p;
		} else {
			desc = p;
		}
		hwzram_impl_free_fifos(hwz,
					 (uint64_t *)desc->dst_addr);

		/* clean up descriptor memory just for paranoia */
		if (hwz->descriptor_type)
			memset(desc1->dst_addr, 0, sizeof(*(desc1->dst_addr)) *
			       ZRAM_NUM_OF_FREE_BLOCKS);
		else
			memset(desc->dst_addr, 0, sizeof(*(desc->dst_addr)) *
			       ZRAM_NUM_OF_FREE_BLOCKS);
	}
}

static void update_complete_index(struct hwzram_impl *hwz);
static void abort_incomplete_descriptors(struct hwzram_impl *hwz)
{
	uint16_t new_complete_index, masked_write_index;
	int i;

	masked_write_index = hwz->write_index & hwz->fifo_index_mask;
	new_complete_index = (hwzram_read_register(hwz, ZRAM_CDESC_CTRL) &
			      ZRAM_CDESC_CTRL_COMPLETE_IDX_MASK) &
		hwz->fifo_index_mask;

	for (i = new_complete_index; i != masked_write_index;
	     i = (i + 1) & hwz->fifo_index_mask) {
		struct hwzram_impl_completion *cmpl = hwz->completions[i];

		if (cmpl) {
			cmpl->compr_status = HWZRAM_ERROR_HALTED;
			cmpl->callback(cmpl);
			hwz->completions[i] = NULL;
		}
	}
}

#if 0
static void hwzram_impl_put_decompr_index(struct hwzram_impl *hwz, int index);
static void hwzram_impl_complete_decompression(struct hwzram_impl *hwz,
					       int index)
{
	int i;
	struct hwzram_impl_completion *cmpl = hwz->decompr_completions[index];
	uint64_t dest_data = hwzram_read_register(hwz, ZRAM_DCMD_DEST(index));
	unsigned int status = ZRAM_DCMD_DEST_STATUS(dest_data);

	WARN_ON(!cmpl);
	cmpl->decompr_status = status;
	atomic64_inc(&hwz->stats.total_decompr_commands);

	pr_devel("%s: [%u] status = %u\n",
		 hwz->name, index, status);

	if (status != HWZRAM_DCMD_DECOMPRESSED) {
		pr_err("%s: bad status 0x%x\n", hwz->name, status);
		hwzram_impl_dump_regs(hwz);
	}

	if (hwz->need_cache_invalidate || hwzram_force_non_coherent) {
		dma_sync_single_range_for_cpu(hwz->dev,
					      cmpl->daddr, 0,
					      UNCOMPRESSED_BLOCK_SIZE,
					      DMA_FROM_DEVICE);
	}

	if (hwzram_non_coherent(hwz)) {
		for (i = 0; i < HWZRAM_MAX_BUFFERS_USED; i++) {
			if (!cmpl->dma_bufs[i])
				continue;

			dma_unmap_single(hwz->dev,
					 ENCODED_ADDR_TO_PHYS(cmpl->dma_bufs[i]),
					 ENCODED_ADDR_TO_SIZE(cmpl->dma_bufs[i]),
					 DMA_TO_DEVICE);
		}
	}

	if (cmpl->daddr) {
		dma_unmap_page(hwz->dev, cmpl->daddr, UNCOMPRESSED_BLOCK_SIZE,
			DMA_FROM_DEVICE);
	}

	hwzram_impl_put_decompr_index(hwz, index);
	cmpl->callback(cmpl);

	/* disable clock */
	if (hwz->hwctrl)
		hwz->hwctrl(DECOMP_DISABLE);
}
#endif

static void hwzram_impl_timer_fn(unsigned long data)
{
	struct hwzram_impl *hwz = (void *)data;
	unsigned long flags;
	u64 compr, error;

	/* enable clock */
	if (hwz->hwctrl) {
		hwz->hwctrl(COMP_ENABLE);
		hwz->hwctrl(DECOMP_ENABLE);
	}

	compr = hwzram_read_register(hwz, ZRAM_INTRP_STS_CMP);
	error = hwzram_read_register(hwz, ZRAM_INTRP_STS_ERROR);

	if (compr)
		hwzram_write_register(hwz, ZRAM_INTRP_STS_CMP, compr);

	spin_lock_irqsave(&hwz->fifo_lock, flags);

	/* if error occurs */
	if (error) {
		hwzram_write_register(hwz, ZRAM_INTRP_STS_ERROR, error);
		error = hwzram_read_register(hwz, ZRAM_ERR_COND);
		pr_err("%s: error condition interrupt non-zero 0x%llx\n",
				hwz->name, (u64)error);
		hwzram_impl_dump_regs(hwz);
		abort_incomplete_descriptors(hwz);
		goto exit;
	}

	if (compr)
		update_complete_index(hwz);

exit:
	spin_unlock_irqrestore(&hwz->fifo_lock, flags);

	/* disable clock */
	if (hwz->hwctrl) {
		hwz->hwctrl(COMP_DISABLE);
		hwz->hwctrl(DECOMP_DISABLE);
	}
}

static irqreturn_t hwzram_impl_irq(int irq, void *data)
{
	pr_devel("%s irq %d\n", __func__, irq);
	hwzram_impl_timer_fn((unsigned long)data);
	return IRQ_HANDLED;
}

/* wait up to a millisecond for reset */
#define HWZRAM_RESET_WAIT_TIME 10
#define HWZRAM_MAX_RESET_WAIT 100

static int hwzram_module_reset(struct hwzram_impl *hwz, unsigned int offset)
{
	uint64_t tmp = (uint64_t)-1ULL;
	unsigned int count = 0;

	hwzram_write_register(hwz, offset + ZRAM_GCTRL, tmp);
	while (count < HWZRAM_MAX_RESET_WAIT &&
	       hwzram_read_register(hwz, offset + ZRAM_GCTRL)) {
		usleep_range(HWZRAM_RESET_WAIT_TIME,
			     HWZRAM_RESET_WAIT_TIME * 2);
		count++;
	}
	if (count == HWZRAM_MAX_RESET_WAIT) {
		pr_warn("%s: timeout waiting for %scompression reset\n",
				hwz->name, offset ? "de" : "");
		return 1;
	}

	return 0;
}

static int hwzram_decomp_reset(struct hwzram_impl *hwz)
{
	return hwzram_module_reset(hwz, ZRAM_DECOMP_OFFSET);
}

static int hwzram_comp_reset(struct hwzram_impl *hwz)
{
	return hwzram_module_reset(hwz, 0);
}

static int hwzram_reset(struct hwzram_impl *hwz)
{
	if (hwzram_comp_reset(hwz))
		return 1;

	if (hwzram_decomp_reset(hwz))
		return 1;

	return 0;
}

static void hwzram_platform_init(struct hwzram_impl *hwz,
				 unsigned int vendor, unsigned int device)
{
	if ((vendor == ZRAM_VENDOR_ID_GOOGLE) &&
	    (device == 0)) {
		/* needed for FPGA bringup */
		hwzram_write_register(hwz, ZRAM_BUSCFG2, 0x0000000033003000LL);
		hwzram_write_register(hwz, ZRAM_BUSCFG, 0x0030030030030000LL);
		hwzram_write_register(hwz, ZRAM_PROT, 0);
		hwzram_write_register(hwz, 0x118, 0x188LL);
	} else
		pr_warn("%s: no proper platform init\n", __func__);
}

static void hwzram_compr_fifo_init(struct hwzram_impl *hwz)
{
	uint64_t data = ffs(hwz->fifo_size) - 1;

	if (!hwz->fifo_fixed) {
		if (hwz->fifo_dma_addr)
			data |= (uint64_t)hwz->fifo_dma_addr;
		else
			data |= (uint64_t)virt_to_phys(hwz->fifo);
	} else {
		int i;
		void *p;
		struct compr_desc_0 *desc;
		struct compr_desc_1 *desc1;

		for (i = 0 ; i < hwz->fifo_size; i++) {
			p = hwz->fifo + (i * hwz->descriptor_size);
			if (hwz->descriptor_type) {
				desc1 = p;
				memset(desc1->dst_addr, 0, sizeof(*(desc1->dst_addr)) *
						ZRAM_NUM_OF_FREE_BLOCKS);
			} else {
				desc = p;
				memset(desc->dst_addr, 0, sizeof(*(desc->dst_addr)) *
						ZRAM_NUM_OF_FREE_BLOCKS);
			}
		}
	}

	hwzram_write_register(hwz, ZRAM_CDESC_LOC, data);
	data = 1ULL << ZRAM_CDESC_CTRL_COMPRESS_ENABLE_SHIFT;
	hwzram_write_register(hwz, ZRAM_CDESC_CTRL, data);
}

struct hwzram_impl *hwzram_get_impl(void)
{
	unsigned long flags;
	struct hwzram_impl *hwz = NULL;

	spin_lock_irqsave(&hwzram_list_lock, flags);
	if (!list_empty(&hwzram_list)) {
		hwz = container_of(hwzram_list.next, struct hwzram_impl,
				   hwzram_list);
		list_del(hwzram_list.next);
		pr_devel("%s: found impl %s\n", __func__, hwz->name);
	} else {
		pr_devel("%s: no hwzram_impl found\n", __func__);
	}
	spin_unlock_irqrestore(&hwzram_list_lock, flags);

	return hwz;
}
EXPORT_SYMBOL(hwzram_get_impl);

void hwzram_return_impl(struct hwzram_impl *hwz)
{
	unsigned long flags;

	spin_lock_irqsave(&hwzram_list_lock, flags);
	list_add_tail(&hwz->hwzram_list, &hwzram_list);
	spin_unlock_irqrestore(&hwzram_list_lock, flags);
}
EXPORT_SYMBOL(hwzram_return_impl);

/* initialize hardware zram block */
struct hwzram_impl *hwzram_impl_init(struct device *dev, uint16_t fifo_size,
				     phys_addr_t regs, int irq,
				     hwzram_platform_initcall platform_init,
				     hwzram_clock_control hwctrl)
{
	struct hwzram_impl *hwz;
	int ret = 0, i;
	uint64_t hwid, hwfeatures, hwfeatures2, decomp_hwfeatures2;
	unsigned int desc_size;
	phys_addr_t fixed_fifo_addr;

	/* Allocate hwzram_impl object */
	hwz = devm_kzalloc(dev, sizeof(*hwz), GFP_KERNEL);
	if (!hwz)
		return ERR_PTR(-ENOMEM);

	hwz->dev = dev;

	/* Apply ioremap */
	hwz->regs = devm_ioremap(hwz->dev, regs, HWZRAM_REGS_SIZE);
	if (!hwz->regs) {
		pr_err("%s: ioremap failed\n", hwz->name);
		ret = -EINVAL;
		goto out_free_object;
	}
	pr_devel("%s: hwzram register is mapped at 0x%p\n", __func__, hwz->regs);

	/*
	 * Parsing HWID, HWFEATURES, HWFEATURES2:
	 * Read registers with "hwz->full_width_reg_access == false".
	 */
	hwid = hwzram_read_register(hwz, ZRAM_HWID);
	hwfeatures = hwzram_read_register(hwz, ZRAM_HWFEATURES);
	hwfeatures2 = hwzram_read_register(hwz, ZRAM_HWFEATURES2);
	decomp_hwfeatures2 =
		hwzram_read_register(hwz, ZRAM_DECOMP_OFFSET + ZRAM_HWFEATURES2);

	pr_devel("%s: hwid(%llu) hwfeatures(%llu) hwfeatures2(%llu) (decomp)hwfeatures2(%llu)\n",
			__func__, hwid, hwfeatures, hwfeatures2, decomp_hwfeatures2);


	if (ZRAM_FEATURES2_FIFOS(hwfeatures2) < 1) {
		pr_err("unexpected zram device with 0 fifos\n");
		ret = -ENODEV;
		goto out_unmap_regs;
	}

	if (ZRAM_FEATURES2_FIFOS(hwfeatures2) > 1)
		pr_warn("Multiple fifos found, currently unsupported\n");

	hwz->descriptor_type = ZRAM_FEATURES2_DESC_TYPE(hwfeatures2);
	hwz->fifo_fixed = ZRAM_FEATURES2_DESC_FIXED(hwfeatures2);
	hwz->max_buffer_count = ZRAM_FEATURES2_BUF_MAX(hwfeatures2);
	hwz->need_cache_flush = ZRAM_FEATURES2_RD_COHERENT(hwfeatures2);
	hwz->need_cache_invalidate = ZRAM_FEATURES2_WR_COHERENT(hwfeatures2);
	/* ZRAM_FEATURES2_RESULT_WRITE(decomp_hwfeatures2); */
	hwz->full_width_reg_access = !!ZRAM_FEATURES2_REG_ACCESS_WIDE(decomp_hwfeatures2);
	hwz->decompr_cmd_count = ZRAM_FEATURES2_DECOMPR_CMDS(decomp_hwfeatures2);

	pr_info("%s: desc_type %u fifo_fixed %u max_bufs %u non-coherent %u full_width_reg_access %s decompress commmand sets %u\n",
		hwz->name, hwz->descriptor_type, hwz->fifo_fixed,
		hwz->max_buffer_count, hwzram_non_coherent(hwz),
		hwz->full_width_reg_access ? "true" : "false",
		hwz->decompr_cmd_count);

	/* Check decriptor size */
	if (hwz->descriptor_type)
		hwz->descriptor_size = sizeof(struct compr_desc_1);
	else
		hwz->descriptor_size = sizeof(struct compr_desc_0);
	desc_size = hwz->descriptor_size;

	/* Set HW related controls */
	if (hwctrl != NULL)
		hwz->hwctrl = hwctrl;

	/* Enable clock */
	if (hwz->hwctrl) {
		hwz->hwctrl(COMP_ENABLE);
		hwz->hwctrl(DECOMP_ENABLE);
	}

	/* Run platform proprietary init function */
	if (platform_init != NULL)
		platform_init(hwz, ZRAM_HWID_VENDOR(hwid), ZRAM_HWID_DEVICE(hwid));
	else
		hwzram_platform_init(hwz, ZRAM_HWID_VENDOR(hwid), ZRAM_HWID_DEVICE(hwid));

	/* Check descriptor fifo */
	if (hwz->fifo_fixed) {
		fixed_fifo_addr = hwzram_read_register(hwz, ZRAM_CDESC_LOC);
		fifo_size = 1 << (fixed_fifo_addr & ZRAM_CDESC_LOC_NUM_DESC_MASK);
		fixed_fifo_addr &= ZRAM_CDESC_LOC_BASE_MASK;
		pr_info("%s: using fixed fifo at %pap with size %u\n", hwz->name,
			&fixed_fifo_addr, fifo_size);
	}

	/* Verify fifo_size is a power of two and less than 32k */
	if (!fifo_size ||
	    ffs(fifo_size) != fls(fifo_size) ||
	    (fifo_size > HWZRAM_MAX_FIFO_SIZE)) {
		pr_err("invalid fifo size %u\n", fifo_size);
		ret = -EINVAL;
		goto out_unmap_regs;
	}

	/* Max count of compressed buffers used */
	if (!hwz->max_buffer_count) {
		pr_err("%s: max buffer count is 0!\n", hwz->name);
		ret = -ENODEV;
		goto out_unmap_regs;
	}

	/* The number of decompression command */
	if (!hwz->decompr_cmd_count) {
		pr_err("%s: error: no decompression engines found!\n",
		       hwz->name);
		ret = -ENODEV;
		goto out_unmap_regs;
	}

	/* Initialize op. fields of hwzram_impl object */
	hwz->fifo_size = fifo_size;
	hwz->fifo_index_mask = fifo_size - 1;
	hwz->fifo_color_mask = (fifo_size << 1) - 1;
	hwz->write_index = hwz->complete_index = 0;
	hwz->completions = devm_kzalloc(dev,
					fifo_size *
					sizeof(struct hwzram_impl_completion *),
					GFP_KERNEL);
	if (!hwz->completions) {
		pr_err("unable to allocate completions array\n");
		ret = -ENOMEM;
		goto out_unmap_regs;
	}

	hwz->decompr_cmd_used = devm_kzalloc(hwz->dev,
					  sizeof(atomic_t) *
					  hwz->decompr_cmd_count,
					  GFP_KERNEL);
	if (!hwz->decompr_cmd_used) {
		pr_err("%s unable to allocate memory for decompression\n",
			hwz->name);
		ret = -ENOMEM;
		goto out_free_op_mem;
	}
	for (i = 0; i < hwz->decompr_cmd_count; i++)
		atomic_set(hwz->decompr_cmd_used + i, 0);

	/* Get a new id */
	hwz->id = ida_simple_get(&hwzram_ida, 0, 0, GFP_KERNEL);
	if (hwz->id < 0) {
		pr_err("unable to get id\n");
		ret = -ENOMEM;
		goto out_free_op_mem;
	}
	snprintf(hwz->name, HWZRAM_IMPL_MAX_NAME, "hwzram%u", hwz->id);
	pr_devel("%s: probing zram device\n", hwz->name);

	/* Driver allocates fifo in regular memory if not fixed fifo */
	if (!hwz->fifo_fixed) {
		/*
		 * If we must do flushes or invalidates we need to use the
		 * dma api to get mappings
		 */
		if (hwzram_non_coherent(hwz)) {
			/* dma non-coherent case */
			init_dma_attrs(&hwz->fifo_dma_attrs);
			dma_set_attr(DMA_ATTR_WRITE_COMBINE, &hwz->fifo_dma_attrs);
			hwz->fifo = dma_alloc_attrs(hwz->dev, fifo_size * desc_size,
					&hwz->fifo_dma_addr, GFP_KERNEL,
					&hwz->fifo_dma_attrs);
			if (!hwz->fifo) {
				pr_err("%s unable to alloc dma fifo\n", hwz->name);
				ret = -ENOMEM;
				goto out_free_id;
			}
			pr_info("%s: fifo_dma_addr = %pad fifo = %p\n",
					hwz->name, &hwz->fifo_dma_addr, hwz->fifo);
		} else {
			/* dma coherent case */
			hwz->fifo_alloc = devm_kzalloc(hwz->dev,
					fifo_size * (desc_size + 1),
					GFP_KERNEL | GFP_DMA);
			if (!hwz->fifo_alloc) {
				pr_err("%s: unable to allocate fifo\n", hwz->name);
				ret = -ENOMEM;
				goto out_free_id;
			}
			hwz->fifo = PTR_ALIGN(hwz->fifo_alloc, desc_size);
			{
				phys_addr_t pfifo = virt_to_phys(hwz->fifo);

				pr_info("%s: fifo is %p phys %pap\n", hwz->name,
						hwz->fifo, &pfifo);
			}
		}
	} else {
		/*
		 * Driver maps in fixed fifo:
		 * TODO(sonnyrao) look at using ioremap_wc() for better performance
		 */
		hwz->fifo = devm_ioremap(hwz->dev, fixed_fifo_addr,
					 fifo_size * desc_size);
		if (!hwz->fifo) {
			pr_err("%s: unable to map HW fifo\n", hwz->name);
			ret = -EIO;
			goto out_free_id;
		}

		pr_devel("%s: Fixed fifo is mapped at 0x%p\n", __func__, hwz->fifo);
	}

	/* Configure IRQ if needed */
	if (irq && !hwzram_force_timer) {
		hwz->irq = irq;
		ret = devm_request_irq(hwz->dev, irq, hwzram_impl_irq,
				       0, hwz->name, hwz);
		if (ret) {
			pr_err("%s: unable to request irq %u ret %d\n",
				hwz->name, irq, ret);
			ret = -EINVAL;
			goto out_free_mem;
		}
	}

	if (!irq || hwzram_force_timer) {
		pr_info("%s: no irq defined, falling back to timer\n",
			hwz->name);
		hwz->need_timer = true;
		setup_timer(&hwz->timer, hwzram_impl_timer_fn,
			    (unsigned long) hwz);
	}

	/* Initialize buffer allocator */
	if (hwzram_allocator_init(&hwz->allocator, hwz->id))
		goto out_free_irq;

	/* Initialize locks */
	spin_lock_init(&hwz->fifo_lock);
	mutex_init(&hwz->cached_bufs_lock);

	/* Add a new hwzram_impl to the list of devices */
	INIT_LIST_HEAD(&hwz->hwzram_list);
	spin_lock(&hwzram_list_lock);
	list_add_tail(&hwz->hwzram_list, &hwzram_list);
	spin_unlock(&hwzram_list_lock);

	/* Reset the block */
	hwzram_reset(hwz);

	/* Set up the fifo and enable */
	hwzram_compr_fifo_init(hwz);

	/* Enable all the interrupts */
	hwzram_write_register(hwz, ZRAM_INTRP_MASK_ERROR, 0ULL);
	hwzram_write_register(hwz, ZRAM_INTRP_MASK_CMP, 0ULL);
	hwzram_write_register(hwz, ZRAM_DECOMP_OFFSET + ZRAM_INTRP_MASK_ERROR, 0ULL);
	hwzram_write_register(hwz, ZRAM_INTRP_MASK_DCMP, 0ULL);

	/* Disable clock */
	if (hwz->hwctrl) {
		hwz->hwctrl(COMP_DISABLE);
		hwz->hwctrl(DECOMP_DISABLE);
	}

	return hwz;

out_free_irq:
	if (hwz->irq)
		devm_free_irq(hwz->dev, hwz->irq, hwz);

out_free_mem:
	if (hwz->fifo_dma_addr)
		dma_free_attrs(hwz->dev, hwz->fifo_size * desc_size,
			       hwz->fifo, hwz->fifo_dma_addr,
			       &hwz->fifo_dma_attrs);

	if (hwz->fifo_alloc)
		devm_kfree(hwz->dev, hwz->fifo_alloc);

	if (hwz->fifo_fixed)
		devm_iounmap(hwz->dev, hwz->fifo);

out_free_id:
	ida_simple_remove(&hwzram_ida, hwz->id);

out_free_op_mem:
	if (hwz->completions)
		devm_kfree(hwz->dev, hwz->completions);

	if (hwz->decompr_cmd_used)
		devm_kfree(hwz->dev, hwz->decompr_cmd_used);

out_unmap_regs:
	devm_iounmap(hwz->dev, hwz->regs);

out_free_object:
	devm_kfree(hwz->dev, hwz);

	return ERR_PTR(ret);
}
EXPORT_SYMBOL(hwzram_impl_init);

void hwzram_impl_destroy(struct hwzram_impl *hwz)
{
	if (hwz->need_timer && timer_pending(&hwz->timer))
		del_timer_sync(&hwz->timer);

	if (hwz->fifo)
		hwzram_impl_fifo_cleanup(hwz);

	hwzram_allocator_destroy(&hwz->allocator);
	if (hwz->irq)
		devm_free_irq(hwz->dev, hwz->irq, hwz);

	if (hwz->fifo_alloc)
		devm_kfree(hwz->dev, hwz->fifo_alloc);
	else if (hwz->fifo_dma_addr) {
		unsigned int desc_size = sizeof(struct compr_desc_0);

		if (hwz->descriptor_type)
			desc_size = sizeof(struct compr_desc_1);
		dma_free_attrs(hwz->dev, hwz->fifo_size * desc_size,
			       hwz->fifo, hwz->fifo_dma_addr,
			       &hwz->fifo_dma_attrs);
	}
	if (hwz->fifo_fixed)
		devm_iounmap(hwz->dev, hwz->fifo);
	devm_iounmap(hwz->dev, hwz->regs);
	ida_simple_remove(&hwzram_ida, hwz->id);
	devm_kfree(hwz->dev, hwz->completions);
	devm_kfree(hwz->dev, hwz);
}
EXPORT_SYMBOL(hwzram_impl_destroy);

static void *update_buf_for_incompressible(struct hwzram_impl *hwz, gfp_t gfp_flags,
					bool fill)
{
#define MAX_BUFS	(4)	/* Suggest to be the max number of CPU cores (in exp. of 2) */
#define BUFS_INDEX_MASK	(MAX_BUFS - 1)
#define BUFS_COLOR_MASK	((MAX_BUFS << 1) - 1)
	static DEFINE_SPINLOCK(lock);
	static void *bufs[MAX_BUFS];
	static uint8_t bget, bput;
	uint8_t diff, index;
	unsigned long flags;
	bool to_fill = false, to_consume = false;
	void *ret;

	/* check the status of bufs */
	spin_lock_irqsave(&lock, flags);
	diff = (bput - bget) & BUFS_COLOR_MASK;
	if (fill) {
		if (diff < MAX_BUFS) {
			index = bput & BUFS_INDEX_MASK;
			bput = (bput + 1) & BUFS_COLOR_MASK;
			to_fill = true;
		}
	} else {
		if (diff > 0) {
			index = bget & BUFS_INDEX_MASK;
			bget = (bget + 1) & BUFS_COLOR_MASK;
			to_consume = true;
		}
	}
	spin_unlock_irqrestore(&lock, flags);

	/* consume bufs */
	if (to_consume && bufs[index] != 0) {
		ret = bufs[index];
		bufs[index] = 0;
		return ret;
	}

	/* if failed to get buf */
	if ((!fill && !to_consume) ||
			(to_consume && bufs[index] == 0))
		return hwzram_alloc_size_noncoherent(hwz, PAGE_SIZE, gfp_flags);

	/* fill bufs */
	if (to_fill && bufs[index] == 0)
		bufs[index] = hwzram_alloc_size_noncoherent(hwz, PAGE_SIZE, gfp_flags);

	return NULL;
}

static void refill_cached_bufs(struct hwzram_impl *hwz, phys_addr_t *bufs)
{
#define FILL_BUFS_GFP_FLAGS	(GFP_NOIO | __GFP_ZERO | __GFP_NOWARN)
	int i, enlarge;
	uint16_t size;
	phys_addr_t addr;

	/* Should we enlarge the buffers */
	enlarge = hwzram_force_no_copy_enable ? 0 : 1;

	/*
	 *  w/ copy enable - 128, 256, 512, 1024, 2048, 4096
	 * w/o copy enable -  64, 128, 256,  512, 1024, 2048
	 */
	for (i = 0; i < ZRAM_NUM_OF_FREE_BLOCKS; i++) {
		if (bufs[i])
			continue;

		size = 1 << (i + ZRAM_MIN_SIZE_SHIFT + enlarge);
retry:
		addr = hwzram_alloc_size(hwz, size, FILL_BUFS_GFP_FLAGS);
		if (addr) {
			bufs[i] = PHYS_ADDR_TO_ENCODED(addr, size);
		} else {
			/* take a break */
			msleep_interruptible(1);

			pr_devel("%s: failed block size (%u)\n", __func__, size);

			/* try a smaller size */
			if (size > ZRAM_MIN_BLOCK_SIZE)
				size >>= 1;

			pr_devel("%s: trying again (%u)\n", __func__, size);
			goto retry;
		}
	}

	/* To handle HWZRAM_ABORT case */
	if (hwzram_force_no_copy_enable)
		update_buf_for_incompressible(hwz, FILL_BUFS_GFP_FLAGS, true);
}

/* maps and flushes page for DMA, returns dma address via src_out */
static int hwzram_cache_flush_src(struct hwzram_impl *hwz,
				  struct page *src_page,
				  dma_addr_t *src_out)
{
	dma_addr_t src = page_to_phys(src_page);

	if (hwz->need_cache_flush || hwzram_force_non_coherent) {
		phys_addr_t old_src = src;

		src = phys_to_dma(hwz->dev, src);
		pr_devel("%s: %s xlate phys %pap to dma %pad\n",
			hwz->name, __func__, &old_src,
			&src);

		src = dma_map_page(hwz->dev, src_page, 0,
				   UNCOMPRESSED_BLOCK_SIZE,
				   DMA_TO_DEVICE);
		if (dma_mapping_error(hwz->dev, src)) {
			pr_err("%s: dma_map_page failed on %pad\n",
			       hwz->name, &old_src);
			return -ENOMEM;
		}
		pr_devel("%s: %s dma_map_page returned %pad\n",
			hwz->name, __func__, &src);
		dma_sync_single_for_device(hwz->dev,
					   src,
					   UNCOMPRESSED_BLOCK_SIZE,
					   DMA_TO_DEVICE);
	}

	*src_out = src;
	return 0;
}

static int setup_desc_1(struct hwzram_impl *hwz, dma_addr_t src,
			struct compr_desc_1 *desc, phys_addr_t *bufs)
{
	int i, empty_dest_bufs = 0;
	u32 value = 0;

	pr_devel("%s: %s: desc = %p src = %pad\n",
		 hwz->name, __func__, desc, &src);

	/* avoid incomplete assignment issue (ex. strb) */
	value = (src >> UNCOMPRESSED_BLOCK_SIZE_LOG2) |
		(HWZRAM_PEND << COMPR_DESC_1_STATUS_SHIFT) |
		(1 << COMPR_DESC_1_INTR_SHIFT);	/* always interrupt */

	desc->u2.value = value;

	for (i = 0; i < ZRAM_NUM_OF_FREE_BLOCKS; i++) {
		if (!desc->dst_addr[i]) {
			if (bufs[i]) {
				desc->dst_addr[i] = bufs[i] >>
					(ZRAM_MIN_SIZE_SHIFT - 1);
				bufs[i] = 0;
			} else {
				empty_dest_bufs++;
			}
		}
	}

	if (empty_dest_bufs == ZRAM_NUM_OF_FREE_BLOCKS)
		return -ENOMEM;

	return 0;
}

static int process_completed_descriptor(struct hwzram_impl *hwz,
					uint16_t fifo_index,
					struct hwzram_impl_completion *cmpl)
{
	struct compr_desc_0 *desc;
	struct compr_desc_1 *desc1 = NULL;
	struct compr_desc_0 tmp;
	phys_addr_t paddr;

	/* we handle type 1 descriptors by converting to type 0 first */
	desc1 = hwz->fifo + (fifo_index * hwz->descriptor_size);
	convert_desc1(&tmp, desc1, cmpl->src_copy);
	desc = &tmp;

	pr_devel("%s: desc 0x%x status 0x%x len %u src %pap\n", hwz->name,
		 fifo_index, desc->u1.s1.status, desc->compr_len,
		 &desc->u1.src_addr);

	cmpl->compr_status = desc->u1.s1.status;
	cmpl->compr_size = desc->compr_len;
#ifdef CONFIG_MTK_ENG_BUILD
	cmpl->hash = desc->hash;
#endif
	memset(cmpl->buffers, 0, sizeof(dma_addr_t) *
	       HWZRAM_MAX_BUFFERS_USED);

	if (hwzram_non_coherent(hwz)) {
		dma_addr_t src = desc->u1.src_addr &
			ZRAM_COMPR_DESC_0_SRC_MASK;
		paddr = dma_to_phys(hwz->dev, src);
		dma_unmap_page(hwz->dev, src,
			       UNCOMPRESSED_BLOCK_SIZE,
			       DMA_TO_DEVICE);
	} else
		paddr = cmpl->src_copy;

	switch (cmpl->compr_status) {
	/* normal case, compression completed successfully or buffer copied */
	case HWZRAM_COPIED:
	case HWZRAM_COMPRESSED:
	{
		unsigned int buf_sel = 0;
		int i;

		buf_sel = desc->buf_sel;
		pr_devel("%s: buf_sel %x\n", __func__, buf_sel);
		for (i = 0; i < hwz->max_buffer_count; i++) {
			unsigned int index = ffs(buf_sel);

			if (index) {
				dma_addr_t addr = desc->dst_addr[index - 1];
				uint16_t size = ENCODED_ADDR_TO_SIZE(addr);

				pr_devel("%s: desc 0x%x used buf %pad (%u)\n",
					hwz->name, fifo_index, &addr, size);

				if (hwzram_non_coherent(hwz)) {
					dma_addr_t daddr = ENCODED_ADDR_TO_PHYS(addr);

					addr = dma_to_phys(hwz->dev, daddr);
					dma_unmap_single(hwz->dev, daddr, size,
							 DMA_FROM_DEVICE);
					addr = PHYS_ADDR_TO_ENCODED(addr, size);
				}
				cmpl->buffers[i] = addr;

				/* clear the entry in the descriptor */
				desc->dst_addr[index - 1] = 0;
				/* if (desc1) */
				desc1->dst_addr[index - 1] = 0;
				buf_sel = buf_sel & ~(1 << (index - 1));
			}
		}
	}
	break;
	/* hardware read the page and found it was all zeros */
	case HWZRAM_ZERO:
		break;
	/* incompressible page or wasn't enough space in dest buffers */
	case HWZRAM_ABORT:
	{
		void *vaddr = NULL;
		void *src_map;
		phys_addr_t dest_paddr = 0;
		int retry = 3;

		/* Select buffer to store incompressible page */
		if (!hwzram_force_no_copy_enable) {
			dma_addr_t addr;
			int index;

			for (index = ZRAM_NUM_OF_FREE_BLOCKS - 1; index >= 0; index--) {
				addr = desc->dst_addr[index];
				if (ENCODED_ADDR_TO_SIZE(addr) == PAGE_SIZE)
					break;
			}

			if (hwzram_non_coherent(hwz)) {
				dma_addr_t daddr = ENCODED_ADDR_TO_PHYS(addr);

				addr = dma_to_phys(hwz->dev, daddr);
				dma_unmap_single(hwz->dev, daddr, PAGE_SIZE, DMA_FROM_DEVICE);
				addr = PHYS_ADDR_TO_ENCODED(addr, PAGE_SIZE);
			}
			dest_paddr = ENCODED_ADDR_TO_PHYS(addr);
			vaddr = kmap_atomic(pfn_to_page(dest_paddr >> PAGE_SHIFT));

			/* clear the entry in the descriptor */
			desc->dst_addr[index]  = 0;
		}

		/* Try other way to get destination buffer */
		while (!vaddr) {
			if (!retry--)
				break;
			vaddr = update_buf_for_incompressible(hwz, GFP_ATOMIC, false);
		}

		if (!vaddr) {
			pr_err("%s: no destination buffer\n", __func__);
			dest_paddr = 0;
			goto exit;
		}

		/* Copy incompressible page to destination, use buffers[0] to store dest page */
		src_map = kmap_atomic(pfn_to_page(paddr >> PAGE_SHIFT));
		if (!src_map) {
			pr_err("%s: kmap_atomic() failed for src_map\n", __func__);
			goto exit;
		}
		memcpy(vaddr, src_map, PAGE_SIZE);
		kunmap_atomic(src_map);
		cmpl->buffers[0] = PHYS_ADDR_TO_ENCODED(virt_to_phys(vaddr), PAGE_SIZE);
		cmpl->compr_status = HWZRAM_COPIED;
		cmpl->compr_size = PAGE_SIZE;

exit:
		/* Unmap if needed */
		if (dest_paddr && !hwzram_force_no_copy_enable)
			kunmap_atomic(vaddr);
	}
	break;

	/* an error occurred, but hardware is still progressing */
	case HWZRAM_ERROR_CONTINUE:
		pr_err("%s: got error on descriptor 0x%x\n", hwz->name,
		       fifo_index);
		break;

	/* a fairly bad error occurred, need to reset the fifo */
	case HWZRAM_ERROR_HALTED:
		pr_err("%s: got fifo error on descriptor 0x%x\n",
		       hwz->name, fifo_index);
		break;

	/*
	 * this shouldn't normally happen -- hardware indicated completed but
	 * descriptor is still in PEND or IDLE.
	 */
	case HWZRAM_IDLE:
	case HWZRAM_PEND:
		pr_err("%s: descriptor 0x%x pend or idle 0x%x: ", hwz->name,
		       fifo_index, cmpl->compr_status);
		{
			int i;
			uint32_t *p =  (uint32_t *)(hwz->fifo +
						  (fifo_index * hwz->descriptor_size));
			for (i = 0; i < (hwz->descriptor_size/sizeof(uint32_t)); i++)
				pr_cont("%08X ", p[i]);
			pr_cont("\n");
		}
		break;
	};

	/* do the callback */
	cmpl->callback(cmpl);

	/* set the descriptor back to IDLE */
	desc->u1.s1.status = HWZRAM_IDLE;
	/* if (desc1) */
	desc1->u2.status.status = HWZRAM_IDLE;

	/* disable clock: the completion of one compression request */
	if (hwz->hwctrl)
		hwz->hwctrl(COMP_DISABLE);

	return 0;
}

static void process_completions(struct hwzram_impl *hwz, uint16_t start,
				uint16_t end)
{
	uint16_t i;

	pr_devel("%s process_completion from 0x%x to 0x%x\n", hwz->name,
		start, end);

	/* mask off color bits */
	start &= hwz->fifo_index_mask;
	end &= hwz->fifo_index_mask;

	for (i = start; i != end; i = ((i + 1) & hwz->fifo_index_mask)) {
		struct hwzram_impl_completion *cmpl = hwz->completions[i];

		process_completed_descriptor(hwz, i, cmpl);
		hwz->completions[i] = NULL;
	}
}

static void update_complete_index(struct hwzram_impl *hwz)
{
	uint16_t new_complete_index = hwzram_read_register(hwz,
							   ZRAM_CDESC_CTRL) &
		ZRAM_CDESC_CTRL_COMPLETE_IDX_MASK;

	if (new_complete_index != hwz->complete_index) {
		process_completions(hwz, hwz->complete_index, new_complete_index);
		hwz->complete_index = new_complete_index;
	}
}

static bool fifo_full(struct hwzram_impl *hwz, uint16_t new_write_index)
{
	uint16_t diff = (new_write_index - hwz->complete_index) &
		hwz->fifo_color_mask;

	if (diff >= hwz->fifo_size) {
		update_complete_index(hwz);
		diff = (new_write_index - hwz->complete_index) &
			hwz->fifo_color_mask;
		if (diff >= hwz->fifo_size)
			return true;
	}

	return false;
}

void hwzram_impl_suspend(struct hwzram_impl *hwz)
{
	/* Clean fifo */
	hwzram_impl_fifo_cleanup(hwz);

	/* Reset index */
	hwz->write_index = hwz->complete_index = 0;
}

void hwzram_impl_resume(struct hwzram_impl *hwz)
{
	/* Reset the block */
	hwzram_reset(hwz);

	/* Set up the fifo and enable */
	hwzram_compr_fifo_init(hwz);

	/* Enable all the interrupts */
	hwzram_write_register(hwz, ZRAM_INTRP_MASK_ERROR, 0ULL);
	hwzram_write_register(hwz, ZRAM_INTRP_MASK_CMP, 0ULL);
	hwzram_write_register(hwz, ZRAM_DECOMP_OFFSET + ZRAM_INTRP_MASK_ERROR, 0ULL);
	hwzram_write_register(hwz, ZRAM_INTRP_MASK_DCMP, 0ULL);
}

int hwzram_impl_compress_page(struct hwzram_impl *hwz, struct page *page,
	struct hwzram_impl_completion *completion)
{
	uint16_t new_write_index;
	unsigned long flags;
	int ret;
	unsigned int desc_size = hwz->descriptor_size;
	void *p;
	dma_addr_t dma_src;

	pr_devel("[%s] %s: starting desc 0x%x for page at 0x%llx\n",
		 current->comm, hwz->name, hwz->write_index,
		 (u64)page_to_phys(page));

	mutex_lock(&hwz->cached_bufs_lock);
	refill_cached_bufs(hwz, hwz->cached_bufs);

	/* enable clock */
	if (hwz->hwctrl)
		hwz->hwctrl(COMP_ENABLE);

	spin_lock_irqsave(&hwz->fifo_lock, flags);

	new_write_index = (hwz->write_index + 1) & hwz->fifo_color_mask;

	if (fifo_full(hwz, new_write_index)) {
		pr_devel("%s: stalling new_write %u complete %u\n",
			 hwz->name, new_write_index,
			 hwz->complete_index);
		hwz->stats.fifo_full_stalls++;
		spin_unlock_irqrestore(&hwz->fifo_lock, flags);
		mutex_unlock(&hwz->cached_bufs_lock);
		return -EBUSY;
	}

	p = hwz->fifo + (desc_size *
			 (hwz->write_index & hwz->fifo_index_mask));
	hwz->completions[hwz->write_index & hwz->fifo_index_mask] = completion;

	/* map source for dma and flush if needed */
	if (hwzram_cache_flush_src(hwz, page, &dma_src)) {
		spin_unlock_irqrestore(&hwz->fifo_lock, flags);
		mutex_unlock(&hwz->cached_bufs_lock);
		pr_err("%s: unable to map source buffer\n",
		       hwz->name);

		return -ENOMEM;
	}
	completion->src_copy = dma_src;

	ret = setup_desc_1(hwz, dma_src, p, hwz->cached_bufs);
	if (ret) {
		spin_unlock_irqrestore(&hwz->fifo_lock, flags);
		mutex_unlock(&hwz->cached_bufs_lock);
		pr_err("%s: not enough memory to submit descriptor\n",
		       hwz->name);

		return -ENOMEM;
	}

	/* write barrier to force writes to be visible everywhere */
	wmb();
	hwzram_write_register(hwz, ZRAM_CDESC_WRIDX, new_write_index);

	pr_devel("%s: desc 0x%x submitted\n", hwz->name, hwz->write_index);
	hwz->write_index = new_write_index;
	hwz->stats.compr_total_commands++;

	spin_unlock_irqrestore(&hwz->fifo_lock, flags);
	mutex_unlock(&hwz->cached_bufs_lock);

	return 0;
}
EXPORT_SYMBOL(hwzram_impl_compress_page);

static int hwzram_impl_get_decompr_index(struct hwzram_impl *hwz,
					 struct hwzram_impl_completion *cmpl)
{
	int i;

	for (i = 0; i < hwz->decompr_cmd_count; i++) {
		if (!atomic_cmpxchg(&hwz->decompr_cmd_used[i], 0, 1)) {
			hwz->decompr_completions[i] = cmpl;
			/* memory barrier */
			smp_wmb();
			return i;
		}
	}

	return -1;
}

static void hwzram_impl_put_decompr_index(struct hwzram_impl *hwz, int index)
{
	hwz->decompr_completions[index] = NULL;
	/* memory barrier */
	smp_wmb();
	atomic_set(&hwz->decompr_cmd_used[index], 0);
}

/* atomic operation */
int hwzram_impl_decompress_page(struct hwzram_impl *hwz,
				struct page *page,
				struct hwzram_impl_completion *cmpl)
{
	int index, retry = 0;
	unsigned int status = 0, i, ret = 0;
	uint64_t csize_data, dest_data;
	dma_addr_t daddr = 0;
	phys_addr_t page_phys = page_to_phys(page);
	uint16_t compr_size = cmpl->compr_size;
	unsigned long timeout;
#ifdef CONFIG_MTK_ENG_BUILD
	u32 hash = cmpl->hash;
#endif

	/* enable clock */
	if (hwz->hwctrl)
		hwz->hwctrl(DECOMP_ENABLE);

	preempt_disable();

	/* loop until we got a valid index */
	do {
		index = hwzram_impl_get_decompr_index(hwz, cmpl);
		if (index < 0) {
			pr_devel("%s: invalid decompression index\n", __func__);
			cpu_relax();
		}
	} while (index < 0);

	pr_devel("%s: cmd set [%u] size 0x%x dest %pa\n",
		 __func__, index, compr_size, &page_phys);

retrydec:
#ifdef CONFIG_MTK_ENG_BUILD
	csize_data = hash;
	csize_data = (csize_data << ZRAM_DCMD_CSIZE_HASH_SHIFT) |
		(compr_size << ZRAM_DCMD_CSIZE_SIZE_SHIFT) |
		0x1;
#else
	csize_data = (uint64_t)compr_size << ZRAM_DCMD_CSIZE_SIZE_SHIFT;
#endif
	dest_data = page_to_phys(page);

	if (hwzram_non_coherent(hwz)) {
		daddr = dest_data = dma_map_page(hwz->dev, page, 0,
						 UNCOMPRESSED_BLOCK_SIZE,
						 DMA_FROM_DEVICE);
		if (dma_mapping_error(hwz->dev, daddr)) {
			pr_err("%s: error mapping page 0x%llx for decompression\n",
			       hwz->name, dest_data);
			ret = -ENOMEM;
			cmpl->decompr_status = HWZRAM_DCMD_ERROR;
			hwzram_impl_put_decompr_index(hwz, index);
			preempt_enable();
			goto out_no_dest;
		}
	}

	dest_data |= ((uint64_t)HWZRAM_DCMD_PEND) << ZRAM_DCMD_DEST_STATUS_SHIFT;

	/* Set timeout for polling */
	timeout = jiffies + 100;

	hwzram_write_register(hwz, ZRAM_DCMD_CSIZE(index), csize_data);

	for (i = 0; i < HWZRAM_MAX_BUFFERS_USED; i++) {
		unsigned int size = ENCODED_ADDR_TO_SIZE(cmpl->dma_bufs[i]);
		uint64_t encoded_size = ffs(size) - ZRAM_MIN_SIZE_SHIFT;
		uint64_t addr = ENCODED_ADDR_TO_PHYS(cmpl->dma_bufs[i]);
		uint64_t data;

		/* Set to 0 for representing "no buffer" */
		if (!cmpl->dma_bufs[i]) {
			hwzram_write_register(hwz, ZRAM_DCMD_BUF0(index) + (0x8 * i), 0);
			continue;
		}

		if (hwzram_non_coherent(hwz)) {
			void *dest = phys_to_virt(addr);

			addr = dma_map_single(hwz->dev, dest, size,
					      DMA_TO_DEVICE);
			if (dma_mapping_error(hwz->dev, addr)) {
				pr_err("%s: error mapping buffer 0x%llx for decompression\n",
				       hwz->name, addr);
				ret = -ENOMEM;
				cmpl->decompr_status = HWZRAM_DCMD_ERROR;
				hwzram_impl_put_decompr_index(hwz, index);
				preempt_enable();
				goto out_dma_unmap;
			}
			cmpl->dma_bufs[i] = PHYS_ADDR_TO_ENCODED(addr, size);
			pr_devel("%s: [%u] dest %p dma_bufs[%u] = %pad (%u)\n",
				hwz->name, index, dest, i, &cmpl->dma_bufs[i],
				size);
		}
		data = addr | encoded_size << ZRAM_DCMD_BUF_SIZE_SHIFT;
		pr_devel("%s [%u] buf %u data = 0x%llx\n", hwz->name, index, i,
			 data);
		hwzram_write_register(hwz, ZRAM_DCMD_BUF0(index) + (0x8 * i),
				      data);
	}

	hwzram_write_register(hwz, ZRAM_DCMD_DEST(index), dest_data);

	do {
		cpu_relax();
		if (time_after(jiffies, timeout)) {
			pr_err("%s: timeout waiting for dcmd %u\n", __func__, index);
			ret = -1;
			break;
		}

		dest_data = hwzram_read_register(hwz, ZRAM_DCMD_DEST(index));
		status = ZRAM_DCMD_DEST_STATUS(dest_data);
	} while (status == HWZRAM_DCMD_PEND);

	cmpl->decompr_status = status;
	pr_devel("%s: [%u] status = %u\n", hwz->name, index, status);

	if (status != HWZRAM_DCMD_DECOMPRESSED) {
		pr_err("%s: bad status 0x%x\n", hwz->name, status);
		ret = -1;

#ifdef CONFIG_MTK_ENG_BUILD
		if (status == HWZRAM_DCMD_HASH) {
			pr_err("%s: mismatched hash 0x%x\n", hwz->name, hash);
			pr_err("%s: 0x1910: 0x%016llX\n", hwz->name, hwzram_read_register(hwz, 0x1910));
			pr_err("%s: 0x1914: 0x%016llX\n", hwz->name, hwzram_read_register(hwz, 0x1914));
		}
#endif
		hwzram_impl_dump_regs(hwz);
		hwzram_impl_dump_regs_for_dec(hwz);
		hwzram_impl_restore_dec(hwz);

		if (retry < 3) {
			ret = 0xFF;
			goto out_dma_unmap;
		}
	}

	if (!ret &&
	    (hwz->need_cache_invalidate || hwzram_force_non_coherent)) {
		dma_sync_single_range_for_cpu(hwz->dev,
					      daddr, 0,
					      UNCOMPRESSED_BLOCK_SIZE,
					      DMA_FROM_DEVICE);
	}

	hwzram_impl_put_decompr_index(hwz, index);
	preempt_enable();

out_dma_unmap:
	if (hwzram_non_coherent(hwz)) {
		for (i = 0; i < HWZRAM_MAX_BUFFERS_USED; i++) {
			if (!cmpl->dma_bufs[i])
				continue;

			dma_unmap_single(hwz->dev,
					 ENCODED_ADDR_TO_PHYS(cmpl->dma_bufs[i]),
					 ENCODED_ADDR_TO_SIZE(cmpl->dma_bufs[i]),
					 DMA_TO_DEVICE);
		}
	}

	if (daddr) {
		dma_unmap_page(hwz->dev, daddr, UNCOMPRESSED_BLOCK_SIZE,
			DMA_FROM_DEVICE);
	}

	/* try to recover decompression error */
	if (ret == 0xFF) {
		retry++;
		ret = 0;
		goto retrydec;
	}

out_no_dest:
	cmpl->callback(cmpl);

	/* disable clock */
	if (hwz->hwctrl)
		hwz->hwctrl(DECOMP_DISABLE);

	return ret;
}
EXPORT_SYMBOL(hwzram_impl_decompress_page);

int hwzram_cache_invl_buf(struct hwzram_impl *hwz, phys_addr_t paddr,
			  size_t size)
{
	dma_addr_t daddr = dma_map_single(hwz->dev,
					  phys_to_virt(paddr),
					  UNCOMPRESSED_BLOCK_SIZE,
					  DMA_FROM_DEVICE);
	if (dma_mapping_error(hwz->dev, daddr)) {
		pr_err("%s: error mapping buffer %pap for copy\n",
		       __func__, &paddr);
		return -ENOMEM;
	}
	dma_sync_single_range_for_cpu(hwz->dev,
					 daddr, 0,
					 UNCOMPRESSED_BLOCK_SIZE,
					 DMA_FROM_DEVICE);
	dma_unmap_single(hwz->dev, daddr, UNCOMPRESSED_BLOCK_SIZE,
				 DMA_FROM_DEVICE);
	return 0;
}

int hwzram_impl_copy_buf(struct hwzram_impl *hwz, struct page *page,
			 phys_addr_t *bufs)
{
	void *dest_map, *src_map;
	phys_addr_t paddr;
	unsigned int i, size = 0, offset, copied_size = 0;
	int ret = 0;

	/* right now we assume the kernel is using 4KiB pages */
	BUILD_BUG_ON(PAGE_SHIFT != UNCOMPRESSED_BLOCK_SIZE_LOG2);

	dest_map = kmap_atomic(page);
	if (!dest_map) {
		pr_err("%s: kmap_atomic() failed for dest\n", __func__);
		return -ENOMEM;
	}

	if (!hwzram_force_no_copy_enable) {
		for (i = 0; i < HWZRAM_MAX_BUFFERS_USED; i++) {
			if (!bufs[i])
				continue;

			paddr = ENCODED_ADDR_TO_PHYS(bufs[i]);
			size = ENCODED_ADDR_TO_SIZE(bufs[i]);
			offset = paddr & (PAGE_SIZE - 1);

			if (hwz->need_cache_invalidate || hwzram_force_non_coherent) {
				if (hwzram_cache_invl_buf(hwz, paddr, size)) {
					pr_err("%s: unable to invalidate %pap (%u)\n",
					       hwz->name, &paddr, size);
					ret = -ENOMEM;
					goto out;
				}
			}

			src_map = kmap_atomic(pfn_to_page(paddr >> PAGE_SHIFT));
			if (!src_map) {
				pr_err("%s: kmap_atomic() failed for src\n", __func__);
				kunmap_atomic(dest_map);
				return -ENOMEM;
			}

			memcpy(dest_map + copied_size, src_map + offset, size);
			copied_size += size;
			kunmap_atomic(src_map);
		}
	} else {
		if (!bufs[0]) {
			ret = -ENOMEM;
			goto out;
		}

		paddr = ENCODED_ADDR_TO_PHYS(bufs[0]);
		size = ENCODED_ADDR_TO_SIZE(bufs[0]);
		src_map = kmap_atomic(pfn_to_page(paddr >> PAGE_SHIFT));
		if (!src_map) {
			pr_err("%s: kmap_atomic() failed for src\n", __func__);
			ret = -ENOMEM;
			goto out;
		}

		memcpy(dest_map + copied_size, src_map, size);
		copied_size += size;
		kunmap_atomic(src_map);
	}

	if (copied_size != UNCOMPRESSED_BLOCK_SIZE) {
		pr_err("%s: %s unexpected total size %u vs %u\n",
		       hwz->name, __func__, size,
			UNCOMPRESSED_BLOCK_SIZE);
		ret = -EINVAL;
	}

out:
	kunmap_atomic(dest_map);

	return ret;
}
EXPORT_SYMBOL(hwzram_impl_copy_buf);

module_param(hwzram_force_non_coherent, bool, 0644);
module_param(hwzram_force_timer, bool, 0644);
module_param(hwzram_force_no_copy_enable, bool, 0644);

MODULE_AUTHOR("Sonny Rao");
MODULE_AUTHOR("Mediatek");
MODULE_DESCRIPTION("Google Hardware Memory compression accelerator");
MODULE_LICENSE("GPL");

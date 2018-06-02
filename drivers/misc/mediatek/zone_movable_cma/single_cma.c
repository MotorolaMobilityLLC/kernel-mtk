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
*/
#define pr_fmt(fmt) "zone_movable_cma: " fmt
#define CONFIG_MTK_ZONE_MOVABLE_CMA_DEBUG

#include <linux/types.h>
#include <linux/of.h>
#include <linux/mm.h>
#include <linux/of_reserved_mem.h>
#include <linux/cma.h>
#include <linux/printk.h>
#include "mt-plat/mtk_meminfo.h"
#include "single_cma.h"

struct cma *cma;
static unsigned long shrunk_pfn;
bool zmc_reserved_mem_inited;

static struct single_cma_registration *single_cma_list[] = {
#ifdef CONFIG_MTK_MEMORY_LOWPOWER
	&memory_lowpower_registration,
#endif
#ifdef CONFIG_MTK_SVP
	&memory_ssvp_registration,
#endif
};


static int zmc_memory_init(struct reserved_mem *rmem)
{
	int ret;
	int nr_registed = ARRAY_SIZE(single_cma_list);
	int i;

	pr_alert("%s, name: %s, base: %pa, size: %pa\n",
		 __func__, rmem->name,
		 &rmem->base, &rmem->size);

	/* init cma area */
	ret = cma_init_reserved_mem(rmem->base, rmem->size, 0, &cma);

	if (ret) {
		pr_err("%s cma failed, ret: %d\n", __func__, ret);
		return 1;
	}
	zmc_reserved_mem_inited = true;

	zmc_base();

	for (i = 0; i < nr_registed; i++) {
		if (single_cma_list[i]->init)
			single_cma_list[i]->init(cma);
	}
	return 0;
}
RESERVEDMEM_OF_DECLARE(zone_movable_cma_init, "mediatek,zone_movable_cma",
			zmc_memory_init);

phys_addr_t zmc_base(void)
{
	int i;
	int entries = ARRAY_SIZE(single_cma_list);
	phys_addr_t alignment;
	phys_addr_t max = 0;
	phys_addr_t max_align = 0;

	for (i = 0; i < entries; i++) {
		pr_info("[%s]: size:%lx, align:%lx\n", single_cma_list[i]->name,
				(unsigned long)single_cma_list[i]->size,
				(unsigned long)single_cma_list[i]->align);

		if (single_cma_list[i]->size > max)
			max = single_cma_list[i]->size;

		if (single_cma_list[i]->align > max_align)
			max_align = single_cma_list[i]->align;
	}

	/* ensure minimal alignment requied by mm core */
	alignment = PAGE_SIZE << max(MAX_ORDER - 1, pageblock_order);

	alignment = max(alignment, max_align);

	if (ALIGN(max, alignment) != max)
		pr_warn("Given unaligned size:%pa", &max);

	max = ALIGN(max, alignment);

	if (cma_get_size(cma) - max > 0) {
		shrunk_pfn = (cma_get_size(cma) - max) >> PAGE_SHIFT;
		pr_info("[Resize-START] ZONE_MOVABLE to size: %pa", &max);
		cma_resize_front(cma, shrunk_pfn);
		pr_info("[Resize-DONE]  ZONE_MOVABLE [0x%lx:0x%lx]\n",
				(unsigned long)(cma_get_base(cma)),
				(unsigned long)(cma_get_base(cma) + cma_get_size(cma)));
	}

	return cma_get_base(cma);
}

struct page *zmc_cma_alloc(struct cma *cma, int count, unsigned int align)
{
	zmc_notifier_call_chain(ZMC_EVENT_ALLOC_MOVABLE, NULL);

	if (!zmc_reserved_mem_inited)
		return cma_alloc(cma, count, align);


	/*
	 * Pre-check with cma bitmap. If there is no enough
	 * memory in zone movable cma, provide error handling
	 * for memory reclaim or abort cma_alloc.
	 */
	if (!cma_alloc_range_ok(cma, count, align)) {
		pr_info("No more space in zone movable cma\n");
		return NULL;
	}

	return cma_alloc(cma, count, align);
}

bool zmc_cma_release(struct cma *cma, struct page *pages, int count)
{

	if (!zmc_reserved_mem_inited)
		return cma_release(cma, pages, count);

	return cma_release(cma, pages, count);
}

static int zmc_resize_and_free_areas(void)
{
	unsigned long base_pfn = __phys_to_pfn(cma_get_base(cma)) - shrunk_pfn;
	unsigned long pfn = base_pfn;
	unsigned nr_to_free_pageblock = shrunk_pfn >> pageblock_order;
	struct zone *zone;
	int i;

	pr_info("Raw input: base:0x%lx, shrunk_pfn:0x%lx\n", (unsigned long)cma_get_base(cma), shrunk_pfn);
	pr_info("start from:0x%lx shrunk_pageblock:%u", (unsigned long)PFN_PHYS(base_pfn), nr_to_free_pageblock);
	zone = page_zone(pfn_to_page(pfn));

	for (i = 0; i < nr_to_free_pageblock; i++) {
		unsigned j;

		base_pfn = pfn;
		for (j = pageblock_nr_pages; j; --j, pfn++) {
			WARN_ON_ONCE(!pfn_valid(pfn));
			/*
			 * alloc_contig_range requires the pfn range
			 * specified to be in the same zone. Make this
			 * simple by forcing the entire CMA resv range
			 * to be in the same zone.
			 */
			if (page_zone(pfn_to_page(pfn)) != zone)
				goto err;
		}
		free_cma_reserved_pageblock(pfn_to_page(base_pfn));
	}
	if (i > 0)
		pr_info("resize ZONE_MOVABLE done!\n");

	return 0;

err:
	return -EINVAL;
}
static int __init zmc_resize(void)
{
	if (!zmc_reserved_mem_inited) {
		pr_alert("uninited cma, start zone_movable_cma fail!\n");
		return -1;
	}

	return zmc_resize_and_free_areas();
}
core_initcall(zmc_resize);

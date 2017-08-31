/*
 * Copyright (c) 2015-2016 MICROTRUST Incorporated
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#ifndef UT_MM_H
#define UT_MM_H

#define ALLOC_NO_WATERMARKS     0x04
#define ALLOC_WMARK_MASK        (ALLOC_NO_WATERMARKS-1)
#define ALLOC_WMARK_LOW         WMARK_LOW
#define ALLOC_CPUSET            0x40
#define ZONE_RECLAIM_NOSCAN     -2
#define ZONE_RECLAIM_FULL       -1
#define ZONE_RECLAIM_SOME       0
#define ZONE_RECLAIM_SUCCESS    1
#define ALLOC_WMARK_MIN         WMARK_MIN
#define UT_MAX_MEM		0xD0000000
#define __ut_alloc_pages(gfp_mask, order) \
	__ut_alloc_pages_node(numa_node_id(), gfp_mask, order)

extern void expand(struct zone *zone, struct page *page,
	int low, int high, struct free_area *area,
	int migratetype);

extern int rmqueue_bulk(struct zone *zone, unsigned int order,
	unsigned long count, struct list_head *list,
	int migratetype, int cold);

extern int prep_new_page(struct page *page, int order, gfp_t gfp_flags);

extern int zlc_zone_worth_trying(struct zonelist *zonelist, struct zoneref *z,
	nodemask_t *allowednodes);

extern nodemask_t *zlc_setup(struct zonelist *zonelist, int alloc_flags);
extern bool zone_allows_reclaim(struct zone *local_zone, struct zone *zone);
extern void zlc_mark_zone_full(struct zonelist *zonelist, struct zoneref *z);

unsigned long ut_get_free_pages(gfp_t gfp_mask, unsigned int order);

#endif /* end of UT_MM_H */

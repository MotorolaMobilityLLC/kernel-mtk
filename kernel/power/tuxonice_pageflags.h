/*
 * kernel/power/tuxonice_pageflags.h
 *
 * Copyright (C) 2004-2014 Nigel Cunningham (nigel at tuxonice net)
 *
 * This file is released under the GPLv2.
 */

#ifndef KERNEL_POWER_TUXONICE_PAGEFLAGS_H
#define KERNEL_POWER_TUXONICE_PAGEFLAGS_H

struct  memory_bitmap;
void memory_bm_clear(struct memory_bitmap *bm);

int mem_bm_set_bit_check(struct memory_bitmap *bm, int index, unsigned long pfn);
void memory_bm_set_bit(struct memory_bitmap *bm, int index, unsigned long pfn);
unsigned long memory_bm_next_pfn(struct memory_bitmap *bm, int index);
unsigned long memory_bm_next_pfn_index(struct memory_bitmap *bm, int index);
void memory_bm_position_reset(struct memory_bitmap *bm);
int toi_alloc_bitmap(struct memory_bitmap **bm);
void toi_free_bitmap(struct memory_bitmap **bm);
void memory_bm_clear(struct memory_bitmap *bm);
void memory_bm_clear_bit(struct memory_bitmap *bm, int index, unsigned long pfn);
void memory_bm_set_bit(struct memory_bitmap *bm, int index, unsigned long pfn);
int memory_bm_test_bit(struct memory_bitmap *bm, int index, unsigned long pfn);
int memory_bm_test_bit_index(struct memory_bitmap *bm, int index, unsigned long pfn);
void memory_bm_clear_bit_index(struct memory_bitmap *bm, int index, unsigned long pfn);

struct toi_module_ops;
int memory_bm_write(struct memory_bitmap *bm, int (*rw_chunk)
	(int rw, struct toi_module_ops *owner, char *buffer, int buffer_size));
int memory_bm_read(struct memory_bitmap *bm, int (*rw_chunk)
	(int rw, struct toi_module_ops *owner, char *buffer, int buffer_size));
int memory_bm_space_needed(struct memory_bitmap *bm);

extern struct memory_bitmap *pageset1_map;
extern struct memory_bitmap *pageset1_copy_map;
extern struct memory_bitmap *pageset2_map;
extern struct memory_bitmap *page_resave_map;
extern struct memory_bitmap *io_map;
extern struct memory_bitmap *nosave_map;
extern struct memory_bitmap *free_map;
extern struct memory_bitmap *compare_map;

#define PagePageset1(page) \
	(pageset1_map && memory_bm_test_bit(pageset1_map, smp_processor_id(), page_to_pfn(page)))
#define SetPagePageset1(page) \
	(memory_bm_set_bit(pageset1_map, smp_processor_id(), page_to_pfn(page)))
#define ClearPagePageset1(page) \
	(memory_bm_clear_bit(pageset1_map, smp_processor_id(), page_to_pfn(page)))

#define PagePageset1Copy(page) \
	(memory_bm_test_bit(pageset1_copy_map, smp_processor_id(), page_to_pfn(page)))
#define SetPagePageset1Copy(page) \
	(memory_bm_set_bit(pageset1_copy_map, smp_processor_id(), page_to_pfn(page)))
#define ClearPagePageset1Copy(page) \
	(memory_bm_clear_bit(pageset1_copy_map, smp_processor_id(), page_to_pfn(page)))

#define PagePageset2(page) \
	(memory_bm_test_bit(pageset2_map, smp_processor_id(), page_to_pfn(page)))
#define SetPagePageset2(page) \
	(memory_bm_set_bit(pageset2_map, smp_processor_id(), page_to_pfn(page)))
#define ClearPagePageset2(page) \
	(memory_bm_clear_bit(pageset2_map, smp_processor_id(), page_to_pfn(page)))

#define PageWasRW(page) \
	(memory_bm_test_bit(pageset2_map, smp_processor_id(), page_to_pfn(page)))
#define SetPageWasRW(page) \
	(memory_bm_set_bit(pageset2_map, smp_processor_id(), page_to_pfn(page)))
#define ClearPageWasRW(page) \
	(memory_bm_clear_bit(pageset2_map, smp_processor_id(), page_to_pfn(page)))

#define PageResave(page) (page_resave_map ? \
	memory_bm_test_bit(page_resave_map, smp_processor_id(), page_to_pfn(page)) : 0)
#define SetPageResave(page) \
	(memory_bm_set_bit(page_resave_map, smp_processor_id(), page_to_pfn(page)))
#define ClearPageResave(page) \
	(memory_bm_clear_bit(page_resave_map, smp_processor_id(), page_to_pfn(page)))

#define PageNosave(page) (nosave_map ? \
	memory_bm_test_bit(nosave_map, smp_processor_id(), page_to_pfn(page)) : 0)
#define SetPageNosave(page) \
	(mem_bm_set_bit_check(nosave_map, smp_processor_id(), page_to_pfn(page)))
#define ClearPageNosave(page) \
	(memory_bm_clear_bit(nosave_map, smp_processor_id(), page_to_pfn(page)))

#define PageNosaveFree(page) (free_map ? \
		memory_bm_test_bit(free_map, smp_processor_id(), page_to_pfn(page)) : 0)
#define SetPageNosaveFree(page) \
	(memory_bm_set_bit(free_map, smp_processor_id(), page_to_pfn(page)))
#define ClearPageNosaveFree(page) \
	(memory_bm_clear_bit(free_map, smp_processor_id(), page_to_pfn(page)))

#define PageCompareChanged(page) (compare_map ? \
		memory_bm_test_bit(compare_map, smp_processor_id(), page_to_pfn(page)) : 0)
#define SetPageCompareChanged(page) \
	(memory_bm_set_bit(compare_map, smp_processor_id(), page_to_pfn(page)))
#define ClearPageCompareChanged(page) \
	(memory_bm_clear_bit(compare_map, smp_processor_id(), page_to_pfn(page)))

extern void save_pageflags(struct memory_bitmap *pagemap);
extern int load_pageflags(struct memory_bitmap *pagemap);
extern int toi_pageflags_space_needed(void);
#endif

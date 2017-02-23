#ifndef __LINUX_PAGE_OWNER_H
#define __LINUX_PAGE_OWNER_H

#ifdef CONFIG_PAGE_OWNER
extern bool page_owner_inited;
extern struct page_ext_operations page_owner_ops;

extern void __reset_page_owner(struct page *page, unsigned int order);
extern void __set_page_owner(struct page *page,
			unsigned int order, gfp_t gfp_mask);
extern int __dump_pfn_backtrace(unsigned long pfn);

static inline void reset_page_owner(struct page *page, unsigned int order)
{
	if (likely(!page_owner_inited))
		return;

	__reset_page_owner(page, order);
}

static inline void set_page_owner(struct page *page,
			unsigned int order, gfp_t gfp_mask)
{
	if (likely(!page_owner_inited))
		return;

	__set_page_owner(page, order, gfp_mask);
}

static inline int dump_pfn_backtrace(unsigned long pfn)
{
	if (likely(!page_owner_inited))
		return 0;

	return __dump_pfn_backtrace(pfn);
}
#ifdef CONFIG_PAGE_OWNER_SLIM

#define BT_HASH_TABLE_SIZE      1801
typedef struct BtEntry {
	struct list_head list;
	size_t nr_entries;
	size_t allocations;
	unsigned long backtrace[8];
} BtEntry;

typedef struct {
	size_t count;
	struct list_head list[BT_HASH_TABLE_SIZE];
} BtTable;
#endif

#else
static inline void reset_page_owner(struct page *page, unsigned int order)
{
}
static inline void set_page_owner(struct page *page,
			unsigned int order, gfp_t gfp_mask)
{
}
static inline int dump_pfn_backtrace(unsigned long pfn)
{
	return 0;
}

#endif /* CONFIG_PAGE_OWNER */
#endif /* __LINUX_PAGE_OWNER_H */

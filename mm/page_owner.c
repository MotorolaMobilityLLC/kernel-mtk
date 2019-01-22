#include <linux/debugfs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/bootmem.h>
#include <linux/stacktrace.h>
#include <linux/page_owner.h>
#include <linux/memblock.h>
#include "internal.h"

#ifdef CONFIG_DEBUG_PAGE_OWNER_DEFAULT_OFF
static bool page_owner_disabled = true;
#else
static bool page_owner_disabled;
#endif

bool page_owner_inited __read_mostly;

static void init_early_allocated_pages(void);

#ifdef CONFIG_PAGE_OWNER_SLIM

#define BOOT_ENTRIES (4 * 1024)

unsigned long bitmap[BITS_TO_LONGS(BOOT_ENTRIES)];

BtTable gBtTable;
static BtEntry memory_base[BOOT_ENTRIES];

static DEFINE_SPINLOCK(page_owner_spin_lock);

static inline size_t get_hash(unsigned long *backtrace, size_t nr_entries)
{
	size_t hash = 0;
	size_t i;

	if (backtrace == NULL)
		return 0;

	for (i = 0 ; i < nr_entries ; i++)
		hash = hash << 5 ^ hash >> 1 ^ backtrace[i] >> 2;

	return hash;
}

BtEntry *alloc_entry(void)
{
	int index;

	index = bitmap_find_free_region(bitmap, BOOT_ENTRIES, 0);
	if (index == -ENOMEM)
		return NULL;

	bitmap_set(bitmap, index, 1);
	return &memory_base[index];
}

static int backtrace_cmp(unsigned long *b1, unsigned long *b2, size_t n)
{
	int i;

	for (i = 0; i < n; i++)
		if (b1[i] != b2[i])
			return b1[i] - b2[i];

	return 0;
}

static BtEntry *find_entry(BtTable *table, int slot,
		unsigned long *backtrace, size_t nr_entry)
{
	BtEntry *entry;

	list_for_each_entry(entry, &table->list[slot], list) {
		/*
		 * See if the entry matches exactly.
		 */
		if (entry->nr_entries == nr_entry) {
			if (!backtrace_cmp(backtrace, entry->backtrace, nr_entry))
				return entry;
		}
	}

	return NULL;
}

static void free_entry(BtEntry *entry)
{
	int index;

	list_del(&entry->list);

	gBtTable.count--;
	index = (int)(entry - &memory_base[0]);
	bitmap_clear(bitmap, index, 1);
}

void release_backtrace(BtEntry *entry, int nr_pages)
{
	unsigned long flags;

	spin_lock_irqsave(&page_owner_spin_lock, flags);
	entry->allocations -= nr_pages;

	if (entry->allocations <= 0)
		free_entry(entry);

	spin_unlock_irqrestore(&page_owner_spin_lock, flags);
}

BtEntry *record_backtrace(unsigned long *backtrace, int nr_pages, size_t nr_entry)
{
	size_t hash = get_hash(backtrace, nr_entry);
	size_t slot = hash % BT_HASH_TABLE_SIZE;
	BtEntry *entry = NULL;
	unsigned long flags;

	spin_lock_irqsave(&page_owner_spin_lock, flags);
	entry = find_entry(&gBtTable, slot, backtrace, nr_entry);

	if (entry != NULL)
		entry->allocations += nr_pages;
	else {
		entry = alloc_entry();

		if (entry == NULL) {
			pr_alert("[ERROR]alloc fails\n");
			pr_alert("No backtrace memory available, disable page owner tracking.\n");
			page_owner_inited = false;
			spin_unlock_irqrestore(&page_owner_spin_lock, flags);
			return NULL;
		}

		entry->allocations = nr_pages;
		entry->nr_entries = nr_entry;

		memcpy(entry->backtrace, backtrace, nr_entry * sizeof(unsigned long));


		list_add(&entry->list, &gBtTable.list[slot]);


		gBtTable.count++;
	}

	spin_unlock_irqrestore(&page_owner_spin_lock, flags);

	return entry;
}

static ssize_t
read_page_owner_slim(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	BtEntry *entry;
	unsigned long flags;
	int ret = 0;
	int index;
	char *kbuf;

	if (!page_owner_inited)
		return -EINVAL;

	index = *ppos;

	if (index >= BOOT_ENTRIES)
		return 0;

	kbuf = kmalloc(count, GFP_KERNEL);

	if (!kbuf)
		return -ENOMEM;

	spin_lock_irqsave(&page_owner_spin_lock, flags);
	/* search memory_base for allocated entries */
	for (; index < BOOT_ENTRIES; index++) {
		entry = &memory_base[index];
		if (entry->allocations > 0) {
			ret = snprintf(kbuf, count,
					"%8zu %pS %p %p %p %p %p %p\n", entry->allocations,
					(void *) entry->backtrace[2], (void *) entry->backtrace[2],
					(void *) entry->backtrace[3], (void *) entry->backtrace[4],
					(void *) entry->backtrace[5], (void *) entry->backtrace[6],
					(void *) entry->backtrace[7]);
			break;
		}
	}

	spin_unlock_irqrestore(&page_owner_spin_lock, flags);

	if (ret >= count) {
		pr_info("Insufficiant buffer\n");
		ret = -1;
		goto error;
	}

	if (copy_to_user(buf, kbuf, ret)) {
		ret = -EFAULT;
		goto error;
	}

	*ppos = index + 1;
	kfree(kbuf);
	return ret;

error:
	kfree(kbuf);
	return ret;
}

static const struct file_operations proc_page_owner_slim_operations = {
	.read		= read_page_owner_slim,
};

#endif /* CONFIG_PAGE_OWNER_SLIM */

static int early_page_owner_param(char *buf)
{
	if (!buf)
		return -EINVAL;

	if (strcmp(buf, "on") == 0)
		page_owner_disabled = false;

	return 0;
}
early_param("page_owner", early_page_owner_param);

static bool need_page_owner(void)
{
	if (page_owner_disabled)
		return false;

	return true;
}

static void init_page_owner(void)
{
#ifdef CONFIG_PAGE_OWNER_SLIM
	int i;
#endif

	if (page_owner_disabled)
		return;

#ifdef CONFIG_PAGE_OWNER_SLIM
	pr_info("Enable [PAGE_OWNER_SLIM]\n");
	for (i = 0; i < BT_HASH_TABLE_SIZE; i++)
		INIT_LIST_HEAD(&gBtTable.list[i]);
#endif

	page_owner_inited = true;
	init_early_allocated_pages();
}

struct page_ext_operations page_owner_ops = {
	.need = need_page_owner,
	.init = init_page_owner,
};

void __reset_page_owner(struct page *page, unsigned int order)
{
	int i;
	struct page_ext *page_ext;

#ifdef CONFIG_PAGE_OWNER_SLIM
	page_ext = lookup_page_ext(page);
	if (page_ext && page_ext->entry) {
		release_backtrace(page_ext->entry, 1 << page_ext->order);
		page_ext->entry = NULL;
	}
#endif
	for (i = 0; i < (1 << order); i++) {
		page_ext = lookup_page_ext(page + i);
		__clear_bit(PAGE_EXT_OWNER, &page_ext->flags);
	}
}

void __set_page_owner(struct page *page, unsigned int order, gfp_t gfp_mask)
{
	struct page_ext *page_ext = lookup_page_ext(page);
#ifdef CONFIG_PAGE_OWNER_SLIM
	BtEntry *entry;
	unsigned long trace_entries[8];
	struct stack_trace trace = {
		.nr_entries = 0,
		.entries = &trace_entries[0],
		.max_entries = ARRAY_SIZE(trace_entries),
		.skip = 3,
	};
#else
	struct stack_trace trace = {
		.nr_entries = 0,
		.max_entries = ARRAY_SIZE(page_ext->trace_entries),
		.entries = &page_ext->trace_entries[0],
		.skip = 3,
	};
#endif

	save_stack_trace(&trace);

#ifdef CONFIG_PAGE_OWNER_SLIM
	entry = record_backtrace(trace.entries, 1 << order, trace.nr_entries);
	page_ext->entry = entry;
#else
	page_ext->nr_entries = trace.nr_entries;
#endif
	page_ext->order = order;
	page_ext->gfp_mask = gfp_mask;

	__set_bit(PAGE_EXT_OWNER, &page_ext->flags);
}

gfp_t __get_page_owner_gfp(struct page *page)
{
	struct page_ext *page_ext = lookup_page_ext(page);

	return page_ext->gfp_mask;
}

static ssize_t
print_page_owner(char __user *buf, size_t count, unsigned long pfn,
		struct page *page, struct page_ext *page_ext)
{
	int ret;
	int pageblock_mt, page_mt;
	char *kbuf;
#ifdef CONFIG_PAGE_OWNER_SLIM
	BtEntry *entry = page_ext->entry;
	struct stack_trace trace = {
		.nr_entries = entry->nr_entries,
		.entries = &entry->backtrace[0],
	};
#else
	struct stack_trace trace = {
		.nr_entries = page_ext->nr_entries,
		.entries = &page_ext->trace_entries[0],
	};
#endif

	kbuf = kmalloc(count, GFP_KERNEL);
	if (!kbuf)
		return -ENOMEM;

	ret = snprintf(kbuf, count,
			"Page allocated via order %u, mask 0x%x\n",
			page_ext->order, page_ext->gfp_mask);

	if (ret >= count)
		goto err;

	/* Print information relevant to grouping pages by mobility */
	pageblock_mt = get_pfnblock_migratetype(page, pfn);
	page_mt  = gfpflags_to_migratetype(page_ext->gfp_mask);
	ret += snprintf(kbuf + ret, count - ret,
			"PFN %lu Block %lu type %d %s Flags %s%s%s%s%s%s%s%s%s%s%s%s\n",
			pfn,
			pfn >> pageblock_order,
			pageblock_mt,
			pageblock_mt != page_mt ? "Fallback" : "        ",
			PageLocked(page)	? "K" : " ",
			PageError(page)		? "E" : " ",
			PageReferenced(page)	? "R" : " ",
			PageUptodate(page)	? "U" : " ",
			PageDirty(page)		? "D" : " ",
			PageLRU(page)		? "L" : " ",
			PageActive(page)	? "A" : " ",
			PageSlab(page)		? "S" : " ",
			PageWriteback(page)	? "W" : " ",
			PageCompound(page)	? "C" : " ",
			PageSwapCache(page)	? "B" : " ",
			PageMappedToDisk(page)	? "M" : " ");

	if (ret >= count)
		goto err;

	ret += snprint_stack_trace(kbuf + ret, count - ret, &trace, 0);
	if (ret >= count)
		goto err;

	ret += snprintf(kbuf + ret, count - ret, "\n");
	if (ret >= count)
		goto err;

	if (copy_to_user(buf, kbuf, ret))
		ret = -EFAULT;

	kfree(kbuf);
	return ret;

err:
	kfree(kbuf);
	return -ENOMEM;
}

static ssize_t
read_page_owner(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	unsigned long pfn;
	struct page *page;
	struct page_ext *page_ext;

	if (!page_owner_inited)
		return -EINVAL;

	page = NULL;
	pfn = min_low_pfn + *ppos;

	/* Find a valid PFN or the start of a MAX_ORDER_NR_PAGES area */
	while (!pfn_valid(pfn) && (pfn & (MAX_ORDER_NR_PAGES - 1)) != 0)
		pfn++;

	drain_all_pages(NULL);

	/* Find an allocated page */
	for (; pfn < max_pfn; pfn++) {
		/*
		 * If the new page is in a new MAX_ORDER_NR_PAGES area,
		 * validate the area as existing, skip it if not
		 */
		if ((pfn & (MAX_ORDER_NR_PAGES - 1)) == 0 && !pfn_valid(pfn)) {
			pfn += MAX_ORDER_NR_PAGES - 1;
			continue;
		}

		/* Check for holes within a MAX_ORDER area */
		if (!pfn_valid_within(pfn))
			continue;

		page = pfn_to_page(pfn);
		if (PageBuddy(page)) {
			unsigned long freepage_order = page_order_unsafe(page);

			if (freepage_order < MAX_ORDER)
				pfn += (1UL << freepage_order) - 1;
			continue;
		}

		page_ext = lookup_page_ext(page);

		/*
		 * Some pages could be missed by concurrent allocation or free,
		 * because we don't hold the zone lock.
		 */
		if (!test_bit(PAGE_EXT_OWNER, &page_ext->flags))
			continue;

		/* Record the next PFN to read in the file offset */
		*ppos = (pfn - min_low_pfn) + 1;

		return print_page_owner(buf, count, pfn, page, page_ext);
	}

	return 0;
}

static void init_pages_in_zone(pg_data_t *pgdat, struct zone *zone)
{
	struct page *page;
	struct page_ext *page_ext;
	unsigned long pfn = zone->zone_start_pfn, block_end_pfn;
	unsigned long end_pfn = pfn + zone->spanned_pages;
	unsigned long count = 0;

	/* Scan block by block. First and last block may be incomplete */
	pfn = zone->zone_start_pfn;

	/*
	 * Walk the zone in pageblock_nr_pages steps. If a page block spans
	 * a zone boundary, it will be double counted between zones. This does
	 * not matter as the mixed block count will still be correct
	 */
	for (; pfn < end_pfn; ) {
		if (!pfn_valid(pfn)) {
			pfn = ALIGN(pfn + 1, MAX_ORDER_NR_PAGES);
			continue;
		}

		block_end_pfn = ALIGN(pfn + 1, pageblock_nr_pages);
		block_end_pfn = min(block_end_pfn, end_pfn);

		page = pfn_to_page(pfn);

		for (; pfn < block_end_pfn; pfn++) {
			if (!pfn_valid_within(pfn))
				continue;

			page = pfn_to_page(pfn);

			/*
			 * We are safe to check buddy flag and order, because
			 * this is init stage and only single thread runs.
			 */
			if (PageBuddy(page)) {
				pfn += (1UL << page_order(page)) - 1;
				continue;
			}

			if (PageReserved(page))
				continue;

			page_ext = lookup_page_ext(page);

			/* Maybe overraping zone */
			if (test_bit(PAGE_EXT_OWNER, &page_ext->flags))
				continue;

			/* Found early allocated page */
			set_page_owner(page, 0, 0);
			count++;
		}
	}

	pr_info("Node %d, zone %8s: page owner found early allocated %lu pages\n",
		pgdat->node_id, zone->name, count);
}

static void init_zones_in_node(pg_data_t *pgdat)
{
	struct zone *zone;
	struct zone *node_zones = pgdat->node_zones;
	unsigned long flags;

	for (zone = node_zones; zone - node_zones < MAX_NR_ZONES; ++zone) {
		if (!populated_zone(zone))
			continue;

		spin_lock_irqsave(&zone->lock, flags);
		init_pages_in_zone(pgdat, zone);
		spin_unlock_irqrestore(&zone->lock, flags);
	}
}

static void init_early_allocated_pages(void)
{
	pg_data_t *pgdat;

	drain_all_pages(NULL);
	for_each_online_pgdat(pgdat)
		init_zones_in_node(pgdat);
}

static const struct file_operations proc_page_owner_operations = {
	.read		= read_page_owner,
};

static int __init pageowner_init(void)
{
	struct dentry *dentry;

	if (!page_owner_inited) {
		pr_info("page_owner is disabled\n");
		return 0;
	}

	dentry = debugfs_create_file("page_owner", S_IRUSR, NULL,
			NULL, &proc_page_owner_operations);
	if (IS_ERR(dentry))
		return PTR_ERR(dentry);

#ifdef CONFIG_PAGE_OWNER_SLIM
	dentry = debugfs_create_file("page_owner_slim", S_IRUSR, NULL,
			NULL, &proc_page_owner_slim_operations);
	if (IS_ERR(dentry))
		return PTR_ERR(dentry);
#endif

	return 0;
}
late_initcall(pageowner_init)

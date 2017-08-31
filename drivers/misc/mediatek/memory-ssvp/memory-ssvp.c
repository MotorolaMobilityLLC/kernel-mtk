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

#define pr_fmt(fmt) "memory-ssvp: " fmt

#include <linux/types.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>
#include <linux/printk.h>
#include <linux/cma.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/mutex.h>
#include <linux/highmem.h>
#include <asm/page.h>
#include <asm-generic/memory_model.h>

#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/memblock.h>
#include <asm/tlbflush.h>
#include "sh_svp.h"

const int is_pre_reserve_memory;

#define _SSVP_MBSIZE_ (CONFIG_MTK_SVP_RAM_SIZE + CONFIG_MTK_TUI_RAM_SIZE)
#define SVP_MBSIZE CONFIG_MTK_SVP_RAM_SIZE
#define TUI_MBSIZE CONFIG_MTK_TUI_RAM_SIZE

#define COUNT_DOWN_MS 10000
#define	COUNT_DOWN_INTERVAL 500

/* 64 MB alignment */
#define SSVP_CMA_ALIGN_PAGE_ORDER 14
#define SSVP_ALIGN_SHIFT (SSVP_CMA_ALIGN_PAGE_ORDER + PAGE_SHIFT)
#define SSVP_ALIGN (1 << SSVP_ALIGN_SHIFT)

static u64 ssvp_upper_limit = UPPER_LIMIT64;

#include <mt-plat/aee.h>
#include "mt-plat/mtk_meminfo.h"

static unsigned long svp_usage_count;
static int ref_count;

enum ssvp_subtype {
	SSVP_SVP,
	SSVP_TUI,
	__MAX_NR_SSVPSUBS,
};

enum svp_state {
	SVP_STATE_DISABLED,
	SVP_STATE_ONING_WAIT,
	SVP_STATE_ONING,
	SVP_STATE_ON,
	SVP_STATE_OFFING,
	SVP_STATE_OFF,
	NR_STATES,
};

const char *const svp_state_text[NR_STATES] = {
	[SVP_STATE_DISABLED]   = "[DISABLED]",
	[SVP_STATE_ONING_WAIT] = "[ONING_WAIT]",
	[SVP_STATE_ONING]      = "[ONING]",
	[SVP_STATE_ON]         = "[ON]",
	[SVP_STATE_OFFING]     = "[OFFING]",
	[SVP_STATE_OFF]        = "[OFF]",
};

struct SSVP_Region {
	unsigned int state;
	unsigned long count;
	bool is_unmapping;
	bool use_cache_memory;
	struct page *page;
	struct page *cache_page;
};

static struct task_struct *_svp_online_task; /* NULL */
static struct cma *cma;
static struct SSVP_Region _svpregs[__MAX_NR_SSVPSUBS];

struct debug_dummy_alloc {
	struct page *chunk_keeper[64];
	int chunk_size;
	int nr_keeper;
	bool is_debug;
};

struct debug_dummy_alloc dummy_alloc = {
	.chunk_size = (4 * SZ_1M) >> PAGE_SHIFT,
};

#define pmd_unmapping(virt, size) set_pmd_mapping(virt, size, 0)
#define pmd_mapping(virt, size) set_pmd_mapping(virt, size, 1)

/*
 * Use for zone-movable-cma callback
 */
void zmc_ssvp_init(struct cma *zmc_cma)
{
	phys_addr_t base = cma_get_base(zmc_cma), size = cma_get_size(zmc_cma);

	cma = zmc_cma;
	pr_info("%s, base: %pa, size: %pa\n", __func__, &base, &size);
}

struct single_cma_registration memory_ssvp_registration = {
	.align = SSVP_ALIGN,
	.size = (_SSVP_MBSIZE_ * SZ_1M),
	.name = "memory-ssvp",
	.init = zmc_ssvp_init,
	.prio = ZMC_SSVP,
};

/*
 * CMA dts node callback function
 */
static int __init memory_ssvp_init(struct reserved_mem *rmem)
{
	int ret;

	pr_info("%s, name: %s, base: 0x%pa, size: 0x%pa\n",
		 __func__, rmem->name,
		 &rmem->base, &rmem->size);

	/* init cma area */
	ret = cma_init_reserved_mem(rmem->base, rmem->size, 0, &cma);

	if (ret) {
		pr_err("%s cma failed, ret: %d\n", __func__, ret);
		return 1;
	}

	return 0;
}
RESERVEDMEM_OF_DECLARE(memory_ssvp, "mediatek,memory-ssvp",
			memory_ssvp_init);

static int __init dedicate_tui_memory(struct reserved_mem *rmem)
{
	struct SSVP_Region *region;

	region = &_svpregs[SSVP_TUI];

	pr_info("%s, name: %s, base: 0x%pa, size: 0x%pa\n",
		 __func__, rmem->name,
		 &rmem->base, &rmem->size);

	region->use_cache_memory = true;
	region->is_unmapping = true;
	region->count = rmem->size / PAGE_SIZE;
	region->cache_page = phys_to_page(rmem->base);

	return 0;
}
RESERVEDMEM_OF_DECLARE(tui_memory, "mediatek,memory-tui",
			dedicate_tui_memory);
/*
 * Check whether memory_ssvp is initialized
 */
static inline bool memory_ssvp_inited(void)
{
	return (cma != NULL);
}

/*
 * memory_ssvp_cma_base - query the cma's base
 */
phys_addr_t memory_ssvp_cma_base(void)
{
	if (cma == NULL)
		return 0;

	return cma_get_base(cma);
}

/*
 * memory_ssvp_cma_size - query the cma's size
 */
phys_addr_t memory_ssvp_cma_size(void)
{
	if (cma == NULL)
		return 0;

	return cma_get_size(cma);
}

#ifdef CONFIG_ARM64
/*
 * Unmapping memory region kernel mapping
 * SSVP protect memory region with EMI MPU. While protecting, memory prefetch
 * will access memory region and trigger warning.
 * To avoid false alarm of protection, We unmap kernel mapping while protecting.
 *
 * @start: start address
 * @size: memory region size
 * @map: 1 for mapping, 0 for unmapping.
 *
 * @return: success return 0, failed return -1;
 */
static int set_pmd_mapping(unsigned long start, phys_addr_t size, int map)
{
	unsigned long address = start;
	pud_t *pud;
	pmd_t *pmd;
	pgd_t *pgd;
	spinlock_t *plt;

	if ((start != (start & PMD_MASK))
			|| (size != (size & PMD_MASK))
			|| !memblock_is_memory(virt_to_phys((void *)start))
			|| !size || !start) {
		pr_info("[invalid parameter]: start=0x%lx, size=%pa\n",
				start, &size);
		return -1;
	}

	pr_debug("start=0x%lx, size=%pa, address=0x%p, map=%d\n",
			start, &size, (void *)address, map);
	while (address < (start + size)) {


		pgd = pgd_offset_k(address);

		if (pgd_none(*pgd) || pgd_bad(*pgd)) {
			pr_info("bad pgd break\n");
			goto fail;
		}

		pud = pud_offset(pgd, address);

		if (pud_none(*pud) || pud_bad(*pud)) {
			pr_info("bad pud break\n");
			goto fail;
		}

		pmd = pmd_offset(pud, address);

		if (pmd_none(*pmd)) {
			pr_info("none ");
			goto fail;
		}

		if (pmd_table(*pmd)) {
			pr_info("pmd_table not set PMD\n");
			goto fail;
		}

		plt = pmd_lock(&init_mm, pmd);
		if (map)
			set_pmd(pmd, (__pmd(*pmd) | PMD_SECT_VALID));
		else
			set_pmd(pmd, (__pmd(*pmd) & ~PMD_SECT_VALID));
		spin_unlock(plt);
		address += PMD_SIZE;
	}

	flush_tlb_all();
	return 0;
fail:
	pr_info("start=0x%lx, size=%pa, address=0x%p, map=%d\n",
			start, &size, (void *)address, map);
	show_pte(NULL, address);
	return -1;
}
#else
static inline int set_pmd_mapping(unsigned long start, phys_addr_t size, int map)
{
	pr_debug("start=0x%lx, size=%pa, map=%d\n", start, &size, map);
	if (!map) {
		pr_info("Flush kmap page table\n");
		kmap_flush_unused();
	}
	return 0;
}
#endif

void ssvp_debug_free_region(void)
{
	struct page *page;
	phys_addr_t page_phys;
	int i;

	for (i = 0; i < dummy_alloc.nr_keeper; i++) {
		page = dummy_alloc.chunk_keeper[i];
		page_phys = page_to_phys(page);

		zmc_cma_release(cma, page, dummy_alloc.chunk_size);

		pr_info("%s[%d]: free chunk_keeper[%d]: (%pa), size: %d\n", __func__, __LINE__,
				i, &page_phys, dummy_alloc.chunk_size);
	}
	dummy_alloc.nr_keeper = 0;
	dummy_alloc.is_debug = false;
}

void ssvp_debug_occupy_region(void)
{
	struct page *page;
	phys_addr_t page_phys;

	while (dummy_alloc.nr_keeper < ARRAY_SIZE(dummy_alloc.chunk_keeper)) {
		page = zmc_cma_alloc(cma, dummy_alloc.chunk_size,
				SSVP_CMA_ALIGN_PAGE_ORDER, &memory_ssvp_registration);

		if (page) {
			page_phys = page_to_phys(page);

			if (page_phys >= UPPER_LIMIT32) {
				zmc_cma_release(cma, page, dummy_alloc.chunk_size);
				break;
			}

			pr_info("%s[%d]: occupy chunk_keeper[%d]: (%pa), size: %d\n", __func__, __LINE__,
					dummy_alloc.nr_keeper, &page_phys, dummy_alloc.chunk_size);


			dummy_alloc.chunk_keeper[dummy_alloc.nr_keeper] = page;
			++dummy_alloc.nr_keeper;
		} else
			break;
	}
	if (dummy_alloc.nr_keeper >= ARRAY_SIZE(dummy_alloc.chunk_keeper)) {
		pr_info("%s[%d]: [error] nr_keeper over length of chunk_keeper, dump and free all.\n",
				__func__, __LINE__);
		ssvp_debug_free_region();
		return;
	}

	pr_info("%s[%d]: occupy done, cost %d cma_blocks\n", __func__, __LINE__, dummy_alloc.nr_keeper);
	dummy_alloc.is_debug = true;
}

static int memory_region_offline(struct SSVP_Region *region,
		phys_addr_t *pa, unsigned long *size, u64 upper_limit)
{
	int offline_retry = 0;
	int ret_map;
	struct page *page;
	phys_addr_t page_phys;

	/* compare with function and system wise upper limit */
	upper_limit = min(upper_limit, ssvp_upper_limit);

	if (region->cache_page)
		page_phys = page_to_phys(region->cache_page);
	else
		page_phys = 0;
	pr_info("%s[%d]: upper_limit: %llx, region{ count: %lu, is_unmapping: %c, use_cache_memory: %c, cache_page: %pa}\n",
			__func__, __LINE__, upper_limit,
			region->count, region->is_unmapping ? 'Y' : 'N',
			region->use_cache_memory ? 'Y' : 'N', &page_phys);

	if (region->use_cache_memory) {
		if (!region->cache_page) {
			pr_info("[NO_CACHE_MEMORY]:\n");
			return -EFAULT;
		}

		page = region->cache_page;
		goto out;
	}

	do {
		pr_info("[SSVP-ALLOCATION]: retry: %d\n", offline_retry);
		page = zmc_cma_alloc(cma, region->count,
				SSVP_CMA_ALIGN_PAGE_ORDER, &memory_ssvp_registration);

		offline_retry++;
		msleep(100);
	} while (page == NULL && offline_retry < 20);

	if (page) {
		phys_addr_t start = page_to_phys(page);
		phys_addr_t end = start + (region->count << PAGE_SHIFT);

		if (end > upper_limit) {
			pr_err("[Reserve Over Limit]: Get region(%pa) over limit(0x%llx)\n",
					&end, upper_limit);
			cma_release(cma, page, region->count);
			page = NULL;
		}
	}

	if (!page)
		return -EBUSY;

	svp_usage_count += region->count;
	ret_map = pmd_unmapping((unsigned long)__va((page_to_phys(page))),
			region->count << PAGE_SHIFT);

	if (ret_map < 0) {
		pr_info("[unmapping fail]: virt:0x%lx, size:0x%lx",
				(unsigned long)__va((page_to_phys(page))),
				region->count << PAGE_SHIFT);

		aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT|DB_OPT_DUMPSYS_ACTIVITY
				| DB_OPT_LOW_MEMORY_KILLER
				| DB_OPT_PID_MEMORY_INFO /*for smaps and hprof*/
				| DB_OPT_PROCESS_COREDUMP
				| DB_OPT_PAGETYPE_INFO
				| DB_OPT_DUMPSYS_PROCSTATS,
				"\nCRDISPATCH_KEY:SVP_SS1\n",
				"[unmapping fail]: virt:0x%lx, size:0x%lx",
				(unsigned long)__va((page_to_phys(page))),
				region->count << PAGE_SHIFT);

		region->is_unmapping = false;
	} else
		region->is_unmapping = true;

out:
	region->page = page;
	if (pa)
		*pa = page_to_phys(page);
	if (size)
		*size = region->count << PAGE_SHIFT;

	return 0;
}

static int memory_region_online(struct SSVP_Region *region)
{
	if (region->use_cache_memory)
		return 0;

	/* remapping if unmapping while offline */
	if (region->is_unmapping) {
		int ret_map;

		ret_map = pmd_mapping((unsigned long)__va((page_to_phys(region->page))),
				region->count << PAGE_SHIFT);

		if (ret_map < 0) {
			pr_info("[remapping fail]: virt:0x%lx, size:0x%lx",
					(unsigned long)__va((page_to_phys(region->page))),
					region->count << PAGE_SHIFT);

			aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT
					|DB_OPT_DUMPSYS_ACTIVITY|DB_OPT_LOW_MEMORY_KILLER
					| DB_OPT_PID_MEMORY_INFO /* for smaps and hprof */
					| DB_OPT_PROCESS_COREDUMP
					| DB_OPT_PAGETYPE_INFO
					| DB_OPT_DUMPSYS_PROCSTATS,
					"\nCRDISPATCH_KEY:SVP_SS1\n",
					"[remapping fail]: virt:0x%lx, size:0x%lx",
					(unsigned long)__va((page_to_phys(region->page))),
					region->count << PAGE_SHIFT);

			region->use_cache_memory = true;
			region->cache_page = region->page;
		} else
			region->is_unmapping = false;
	}

	if (!region->is_unmapping && !region->use_cache_memory) {
		zmc_cma_release(cma, region->page, region->count);
		svp_usage_count -= region->count;
	}

	region->page = NULL;
	return 0;
}

int _tui_region_offline(phys_addr_t *pa, unsigned long *size,
		u64 upper_limit)
{
	int retval = 0;
	struct SSVP_Region *region = &_svpregs[SSVP_TUI];

	pr_info("%s %d: >>>>>> state: %s, upper_limit:0x%llx\n", __func__, __LINE__,
			svp_state_text[region->state], upper_limit);

	if (region->state != SVP_STATE_ON) {
		retval = -EBUSY;
		goto out;
	}

	region->state = SVP_STATE_OFFING;
	retval = memory_region_offline(region, pa, size, upper_limit);
	if (retval < 0) {
		region->state = SVP_STATE_ON;
		retval = -EAGAIN;
		goto out;
	}

	region->state = SVP_STATE_OFF;
	pr_info("%s %d: [reserve done]: pa 0x%lx, size 0x%lx\n",
			__func__, __LINE__, (unsigned long)page_to_phys(region->page),
			region->count << PAGE_SHIFT);

out:
	pr_info("%s %d: <<<<<< state: %s, retval: %d\n", __func__, __LINE__,
			svp_state_text[region->state], retval);
	return retval;
}

int tui_region_online(void)
{
	struct SSVP_Region *region = &_svpregs[SSVP_TUI];
	int retval;

	pr_info("%s %d: >>>>>> enter state: %s\n", __func__, __LINE__,
			svp_state_text[region->state]);

	if (region->state != SVP_STATE_OFF) {
		retval = -EBUSY;
		goto out;
	}

	region->state = SVP_STATE_ONING_WAIT;
	retval = memory_region_online(region);
	region->state = SVP_STATE_ON;

out:
	pr_info("%s %d: <<<<<< leave state: %s, retval: %d\n",
			__func__, __LINE__, svp_state_text[region->state], retval);
	return retval;
}
EXPORT_SYMBOL(tui_region_online);

static int _svp_wdt_kthread_func(void *data)
{
	int count_down = COUNT_DOWN_MS / COUNT_DOWN_INTERVAL;

	pr_info("[START COUNT DOWN]: %dms/%dms\n", COUNT_DOWN_MS, COUNT_DOWN_INTERVAL);

	for (; count_down > 0; --count_down) {
		msleep(COUNT_DOWN_INTERVAL);

		/*
		 * some component need ssvp memory,
		 * and stop count down watch dog
		 */
		if (ref_count > 0) {
			_svp_online_task = NULL;
			pr_info("[STOP COUNT DOWN]: new request for ssvp\n");
			return 0;
		}

		if (_svpregs[SSVP_SVP].state == SVP_STATE_ON) {
			pr_info("[STOP COUNT DOWN]: SSVP has online\n");
			return 0;
		}
		pr_info("[COUNT DOWN]: %d\n", count_down);

	}
	pr_info("[COUNT DOWN FAIL]\n");

	pr_info("Shareable SVP trigger kernel warnin");
	aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT|DB_OPT_DUMPSYS_ACTIVITY|DB_OPT_LOW_MEMORY_KILLER
			| DB_OPT_PID_MEMORY_INFO /*for smaps and hprof*/
			| DB_OPT_PROCESS_COREDUMP
			| DB_OPT_DUMPSYS_SURFACEFLINGER
			| DB_OPT_DUMPSYS_GFXINFO
			| DB_OPT_DUMPSYS_PROCSTATS,
			"\nCRDISPATCH_KEY:SVP_SS1\n",
			"[SSVP ONLINE FAIL]: online timeout due to none free memory region.\n");

	return 0;
}

int svp_start_wdt(void)
{
	_svp_online_task = kthread_create(_svp_wdt_kthread_func, NULL, "svp_online_kthread");
	wake_up_process(_svp_online_task);
	return 0;
}

int _svp_region_offline(phys_addr_t *pa, unsigned long *size,
		u64 upper_limit)
{
	int retval = 0;
	struct SSVP_Region *region = &_svpregs[SSVP_SVP];

	pr_info("%s %d: >>>>>> state: %s, upper_limit:0x%llx\n", __func__, __LINE__,
			svp_state_text[region->state], upper_limit);

	if (region->state != SVP_STATE_ON) {
		retval = -EBUSY;
		goto out;
	}

	region->state = SVP_STATE_OFFING;
	retval = memory_region_offline(region, pa, size, upper_limit);
	if (retval < 0) {
		region->state = SVP_STATE_ON;
		retval = -EAGAIN;
		goto out;
	}

	region->state = SVP_STATE_OFF;
	pr_info("%s %d: [reserve done]: pa 0x%lx, size 0x%lx\n",
			__func__, __LINE__, (unsigned long)page_to_phys(region->page),
			region->count << PAGE_SHIFT);

out:
	pr_info("%s %d: <<<<<< state: %s, retval: %d\n", __func__, __LINE__,
			svp_state_text[region->state], retval);
	return retval;
}

int svp_region_online(void)
{
	struct SSVP_Region *region = &_svpregs[SSVP_SVP];
	int retval;

	pr_info("%s %d: >>>>>> enter state: %s\n", __func__, __LINE__,
			svp_state_text[region->state]);

	if (region->state != SVP_STATE_OFF) {
		retval = -EBUSY;
		goto out;
	}

	region->state = SVP_STATE_ONING_WAIT;
	retval = memory_region_online(region);
	region->state = SVP_STATE_ON;

out:
	pr_info("%s %d: <<<<<< leave state: %s, retval: %d\n",
			__func__, __LINE__, svp_state_text[region->state], retval);
	return retval;
}
EXPORT_SYMBOL(svp_region_online);

static long svp_cma_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	pr_info("%s %d: cmd: %x\n", __func__, __LINE__, cmd);

	switch (cmd) {
	case SVP_REGION_IOC_ONLINE:
		pr_info("Called phased out ioctl: %s\n", "SVP_REGION_IOC_ONLINE");
		/* svp_region_online(); */
		break;
	case SVP_REGION_IOC_OFFLINE:
		pr_info("Called phased out ioctl: %s\n", "SVP_REGION_IOC_OFFLINE");
		/* svp_region_offline(NULL, NULL); */
		break;
	case SVP_REGION_ACQUIRE:
		if (ref_count == 0 && _svp_online_task != NULL)
			_svp_online_task = NULL;

		ref_count++;
		break;
	case SVP_REGION_RELEASE:
		ref_count--;

		if (ref_count == 0)
			svp_start_wdt();
		break;
	default:
		return -ENOTTY;
	}

	return ret;
}

#if IS_ENABLED(CONFIG_COMPAT)
static long svp_cma_COMPAT_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	pr_info("%s %d: cmd: %x\n", __func__, __LINE__, cmd);

	if (!filp->f_op || !filp->f_op->unlocked_ioctl)
		return -ENOTTY;

	return filp->f_op->unlocked_ioctl(filp, cmd,
							(unsigned long)compat_ptr(arg));
}

#else

#define svp_cma_COMPAT_ioctl  NULL

#endif

static const struct file_operations svp_cma_fops = {
	.owner =    THIS_MODULE,
	.unlocked_ioctl = svp_cma_ioctl,
	.compat_ioctl   = svp_cma_COMPAT_ioctl,
};
static ssize_t memory_ssvp_write(struct file *file, const char __user *user_buf,
		size_t size, loff_t *ppos)
{
	char buf[64];
	int buf_size;

	buf_size = min(size, (sizeof(buf) - 1));
	if (strncpy_from_user(buf, user_buf, buf_size) < 0)
		return -EFAULT;
	buf[buf_size] = 0;

	pr_info("%s[%d]: cmd> %s\n", __func__, __LINE__, buf);
	if (strncmp(buf, "svp=on", 6) == 0)
		svp_region_online();
	else if (strncmp(buf, "svp=off", 7) == 0)
		_svp_region_offline(NULL, NULL, ssvp_upper_limit);
	else if (strncmp(buf, "tui=on", 6) == 0)
		tui_region_online();
	else if (strncmp(buf, "tui=off", 7) == 0)
		_tui_region_offline(NULL, NULL, ssvp_upper_limit);
	else if (strncmp(buf, "32mode", 6) == 0)
		ssvp_upper_limit = 0x100000000ULL;
	else if (strncmp(buf, "64mode", 6) == 0)
		ssvp_upper_limit = UPPER_LIMIT64;
	else if (strncmp(buf, "debug_64only=on", 15) == 0) {
		ssvp_upper_limit = UPPER_LIMIT64;
		ssvp_debug_occupy_region();
	} else if (strncmp(buf, "debug_64only=off", 16) == 0)
		ssvp_debug_free_region();
	else
		return -EINVAL;

	*ppos += size;
	return size;
}

static int memory_ssvp_show(struct seq_file *m, void *v)
{
	phys_addr_t cma_base = cma_get_base(cma);
	phys_addr_t cma_end = cma_base + cma_get_size(cma);

	seq_printf(m, "cma info: [%pa-%pa] (0x%lx)\n",
			&cma_base, &cma_end,
			cma_get_size(cma));

	seq_printf(m, "cma info: base %pa pfn [%lu-%lu] count %lu\n",
			&cma_base,
			__phys_to_pfn(cma_base), __phys_to_pfn(cma_end),
			cma_get_size(cma) >> PAGE_SHIFT);

	seq_printf(m, "svp region base:0x%lx, count %lu, state %s.%s\n",
			_svpregs[SSVP_SVP].page == NULL ? 0 : (unsigned long)page_to_phys(_svpregs[SSVP_SVP].page),
			_svpregs[SSVP_SVP].count,
			svp_state_text[_svpregs[SSVP_SVP].state],
			_svpregs[SSVP_SVP].use_cache_memory ? " (cache memory)" : "");

	seq_printf(m, "tui region base:0x%lx, count %lu, state %s.%s\n",
			_svpregs[SSVP_TUI].page == NULL ? 0 : (unsigned long)page_to_phys(_svpregs[SSVP_TUI].page),
			_svpregs[SSVP_TUI].count,
			svp_state_text[_svpregs[SSVP_TUI].state],
			_svpregs[SSVP_TUI].use_cache_memory ? " (cache memory)" : "");

	seq_printf(m, "cma usage: %lu pages\n", svp_usage_count);

	seq_puts(m, "[CONFIG]:\n");
	seq_printf(m, "ssvp_upper_limit: 0x%llx\n", ssvp_upper_limit);
	if (dummy_alloc.is_debug) {
		int i;
		phys_addr_t page_phys;

		seq_printf(m, "dummy_alloc.nr_keeper[%d], size[%d]  = {\n",
				dummy_alloc.nr_keeper, dummy_alloc.chunk_size);
		for (i = 0; i < dummy_alloc.nr_keeper; i++) {
			page_phys = page_to_phys(dummy_alloc.chunk_keeper[i]);
			seq_printf(m, "\tpage: %pa,\n", &page_phys);
		}
		seq_puts(m, "}\n");
	}
	return 0;
}

static int memory_ssvp_open(struct inode *inode, struct file *file)
{
	return single_open(file, &memory_ssvp_show, NULL);
}

static const struct file_operations memory_ssvp_fops = {
	.open		= memory_ssvp_open,
	.read		= seq_read,
	.write		= memory_ssvp_write,
	.release	= single_release,
};

static int ssvp_sanity(void)
{
	phys_addr_t start;
	unsigned long size;
	char *err_msg = NULL;

	if (!cma) {
		pr_err("[INIT FAIL]: cma is not inited\n");
		err_msg = "SVP sanity: CMA init fail\nCRDISPATCH_KEY:SVP_SS1";
		goto out;
	}


	start = cma_get_base(cma);
	size = cma_get_size(cma);

	if (start + (_SSVP_MBSIZE_ * SZ_1M) > ssvp_upper_limit) {
		pr_err("[INVALID REGION]: CMA PA over 32 bit\n");
		pr_info("MBSIZE: %d, cma start: %pa, size:0x%lx\n", _SSVP_MBSIZE_, &start, size);
		err_msg = "SVP sanity: invalid CMA region due to over 32bit\nCRDISPATCH_KEY:SVP_SS1";
		goto out;
	}

	return 0;

out:
	aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT|DB_OPT_DUMPSYS_ACTIVITY
			| DB_OPT_PID_MEMORY_INFO /*for smaps and hprof*/
			| DB_OPT_DUMPSYS_PROCSTATS,
			"\nCRDISPATCH_KEY:SVP_SS1\n",
			err_msg);
	return -1;
}


int memory_ssvp_init_region(char *name, int size, struct SSVP_Region *region,
		const struct file_operations *entry_fops)
{
	struct proc_dir_entry *procfs_entry;
	bool has_dedicate_memory = (region->use_cache_memory);
	bool has_region = (has_dedicate_memory || size > 0);

	if (!has_region) {
		region->state = SVP_STATE_DISABLED;
		return -1;
	}

	if (entry_fops) {
		procfs_entry = proc_create(name, 0, NULL, entry_fops);
		if (!procfs_entry) {
			pr_info("Failed to create procfs svp_region file\n");
			return -1;
		}
	}

	if (has_dedicate_memory) {
		pr_info("[%s]:Use dedicate memory as cached memory\n", name);
		goto region_init_done;
	}

	region->count = (size * SZ_1M) >> PAGE_SHIFT;

	if (is_pre_reserve_memory) {
		int ret_map;
		struct page *page;

		page = zmc_cma_alloc(cma, region->count,
				SSVP_CMA_ALIGN_PAGE_ORDER, &memory_ssvp_registration);
		region->use_cache_memory = true;
		region->cache_page = page;
		svp_usage_count += region->count;
		ret_map = pmd_unmapping((unsigned long)__va((page_to_phys(region->cache_page))),
				region->count << PAGE_SHIFT);

		if (ret_map < 0) {
			pr_info("[unmapping fail]: virt:0x%lx, size:0x%lx",
					(unsigned long)__va((page_to_phys(page))),
					region->count << PAGE_SHIFT);

			aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT|DB_OPT_DUMPSYS_ACTIVITY
					| DB_OPT_LOW_MEMORY_KILLER
					| DB_OPT_PID_MEMORY_INFO /*for smaps and hprof*/
					| DB_OPT_PROCESS_COREDUMP
					| DB_OPT_PAGETYPE_INFO
					| DB_OPT_DUMPSYS_PROCSTATS,
					"\nCRDISPATCH_KEY:SVP_SS1\n",
					"[unmapping fail]: virt:0x%lx, size:0x%lx",
					(unsigned long)__va((page_to_phys(page))),
					region->count << PAGE_SHIFT);

			region->is_unmapping = false;
		} else
			region->is_unmapping = true;
	}

region_init_done:
	region->state = SVP_STATE_ON;
	pr_info("%s %d: %s is enable with size: %d mB\n",
			__func__, __LINE__, name, size);
	return 0;
}

static int __init memory_ssvp_debug_init(void)
{
	struct dentry *dentry;

	if (ssvp_sanity() < 0) {
		pr_err("SSVP sanity fail\n");
		return 1;
	}

	pr_info("[PASS]: SSVP sanity.\n");

	dentry = debugfs_create_file("memory-ssvp", S_IRUGO, NULL, NULL, &memory_ssvp_fops);
	if (!dentry)
		pr_warn("Failed to create debugfs memory_ssvp file\n");

	memory_ssvp_init_region("svp_region", SVP_MBSIZE, &_svpregs[SSVP_SVP], &svp_cma_fops);
	memory_ssvp_init_region("tui_region", TUI_MBSIZE, &_svpregs[SSVP_TUI], NULL);

	return 0;
}
late_initcall(memory_ssvp_debug_init);

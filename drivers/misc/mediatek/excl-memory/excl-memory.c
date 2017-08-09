#define pr_fmt(fmt) "excl-memory: " fmt

#include <linux/types.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>
#include <linux/printk.h>
#include <linux/cma.h>
#include <linux/debugfs.h>
#include <linux/stat.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/memblock.h>
#include <asm/page.h>
#include <asm-generic/memory_model.h>
#include <mach/emi_mpu.h>

static struct cma *cma;
static DEFINE_MUTEX(excl_memory_mutex);

/*
 * Check whether excl_memory is initialized
 */
bool excl_memory_inited(void)
{
	return (cma != NULL);
}

/*
 * excl_memory_cma_base - query the cma's base
 */
unsigned long excl_memory_cma_base(void)
{
	return __phys_to_pfn(cma_get_base(cma));
}

/*
 * excl_memory_cma_size - query the cma's size
 */
unsigned long excl_memory_cma_size(void)
{
	return cma_get_size(cma) >> PAGE_SHIFT;
}

/*
 * get_excl_memory - allocate aligned cma memory for exclusive use
 * @count: Requested number of pages.
 * @align: Requested alignment of pages (in PAGE_SIZE order).
 * @pages: Pointer indicates allocated cma buffer.
 * It returns 0 is success, otherwise returns -1
 */
int get_excl_memory(int count, unsigned int align, struct page **pages)
{
	int ret = 0;

	mutex_lock(&excl_memory_mutex);

	*pages = cma_alloc(cma, count, align);
	if (*pages == NULL) {
		pr_alert("lowpower cma allocation failed\n");
		ret = -1;
	}

	mutex_unlock(&excl_memory_mutex);

	return ret;
}

/*
 * put_excl_memory - free aligned cma memory
 * @count: Requested number of pages.
 * @pages: Pointer indicates allocated cma buffer.
 * It returns 0 is success, otherwise returns -ENOMEM
 */
int put_excl_memory(int count, struct page *pages)
{
	int ret;

	mutex_lock(&excl_memory_mutex);

	if (pages) {
		ret = cma_release(cma, pages, count);
		if (!ret) {
			pr_err("%s incorrect pages: %p(%lx)\n",
					__func__, pages, page_to_pfn(pages));
			return -EINVAL;
		}
	}

	mutex_unlock(&excl_memory_mutex);

	return 0;
}

static int excl_memory_init(struct reserved_mem *rmem)
{
	int ret;

	pr_alert("%s, name: %s, base: 0x%pa, size: 0x%pa\n",
		 __func__, rmem->name, &rmem->base, &rmem->size);

	/* init cma area for exclusive use */
	ret = cma_init_reserved_mem(rmem->base, rmem->size , 0, &cma);
	if (ret) {
		pr_err("%s cma failed, ret: %d\n", __func__, ret);
		return 1;
	}

	return 0;
}
RESERVEDMEM_OF_DECLARE(excl_memory, "mediatek,excl-memory", excl_memory_init);

/*
 * Let pages be skipped by compaction
 */
static void set_memory_skip(unsigned long pfn, unsigned long count)
{
	struct page *page;
	unsigned long i;

	for (i = 0; i < count; i += pageblock_nr_pages, pfn += pageblock_nr_pages) {
		page = pfn_to_page(pfn);
		set_pageblock_skip(page);
	}
}

/*
 * Set EMI_MPU protection
 */
static void protect_excl_memory(unsigned long pfn, unsigned long count)
{
	unsigned int sec_mem_mpu_attr = SET_ACCESS_PERMISSON(
			FORBIDDEN, SEC_R_NSEC_R, FORBIDDEN, FORBIDDEN,
			FORBIDDEN, FORBIDDEN, FORBIDDEN, NO_PROTECTION);
	unsigned int set_mpu_ret = 0;

	set_mpu_ret = emi_mpu_set_region_protection(PFN_PHYS(pfn),
			PFN_PHYS(pfn) + (count << PAGE_SHIFT) - 1, 21, sec_mem_mpu_attr);
	if (set_mpu_ret)
		pr_alert("%s fail\n", __func__);
}

static int __init excl_memory_late_init(void)
{
	if (!excl_memory_inited())
		goto out;

	/* Set memory skip */
	set_memory_skip(excl_memory_cma_base(), excl_memory_cma_size());

	/* Protect memory */
	protect_excl_memory(excl_memory_cma_base(), excl_memory_cma_size());
out:
	return 0;
}
late_initcall_sync(excl_memory_late_init);

#ifdef CONFIG_MTK_EXCL_MEMORY_DEBUG
static int excl_memory_show(struct seq_file *m, void *v)
{
	phys_addr_t cma_base = cma_get_base(cma);
	phys_addr_t cma_end = cma_base + cma_get_size(cma);

	seq_printf(m, "cma info: [%pa-%pa] (0x%lx)\n",
			&cma_base, &cma_end, cma_get_size(cma));

	return 0;
}

static int excl_memory_open(struct inode *inode, struct file *file)
{
	return single_open(file, &excl_memory_show, NULL);
}

static ssize_t excl_memory_write(struct file *file, const char __user *buffer,
					size_t count, loff_t *ppos)
{
	static char state;
	static struct page *page;

	if (count > 0) {
		if (get_user(state, buffer))
			return -EFAULT;
		state -= '0';
		pr_alert("%s state = %d\n", __func__, state);
		if (state) {
			/* collect cma */
			get_excl_memory(1, 0, &page);
			pr_alert("%s [0x%lx] [%lu]\n", __func__, page_to_pfn(page), get_pageblock_skip(page));
		} else {
			/* undo collection */
			put_excl_memory(1, page);
			pr_alert("%s [0x%lx] [%lu]\n", __func__, page_to_pfn(page), get_pageblock_skip(page));
		}
	}

	return count;
}

static const struct file_operations excl_memory_fops = {
	.open		= excl_memory_open,
	.write		= excl_memory_write,
	.read		= seq_read,
	.release	= single_release,
};

static int __init excl_memory_debug_init(void)
{
	struct dentry *dentry;

	if (!excl_memory_inited()) {
		pr_err("excl-memory cma is not inited\n");
		return 1;
	}

	dentry = debugfs_create_file("excl-memory", S_IRUGO, NULL, NULL,
					&excl_memory_fops);
	if (!dentry)
		pr_warn("Failed to create debugfs excl_memory_debug_init file\n");

	return 0;
}

late_initcall(excl_memory_debug_init);
#endif /* CONFIG_MTK_EXCL_MEMORY_DEBUG */

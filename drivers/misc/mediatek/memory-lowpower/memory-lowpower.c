#define pr_fmt(fmt) "memory-lowpower: " fmt
#define CONFIG_MTK_MEMORY_LOWPOWER_DEBUG

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
#include <asm/page.h>
#include <asm-generic/memory_model.h>

static struct cma *cma;
static struct page *cma_pages;
static DEFINE_MUTEX(memory_lowpower_mutex);

/*
 * get_memory_lowpwer_cma - allocate all cma memory belongs to lowpower cma
 *
 */
int get_memory_lowpower_cma(void)
{
	int count = cma_get_size(cma) >> PAGE_SHIFT;

	if (cma_pages) {
		pr_alert("cma already collected\n");
		goto out;
	}

	mutex_lock(&memory_lowpower_mutex);

	cma_pages = cma_alloc(cma, count, 0);

	if (cma_pages)
		pr_debug("%s:%d ok\n", __func__, __LINE__);
	else
		pr_alert("lowpower cma allocation failed\n");

	mutex_unlock(&memory_lowpower_mutex);

out:
	return 0;
}

/*
 * put_memory_lowpwer_cma - free all cma memory belongs to lowpower cma
 *
 * It returns 0 is success, otherwise returns -ENOMEM
 */
int put_memory_lowpower_cma(void)
{
	int ret;
	int count = cma_get_size(cma) >> PAGE_SHIFT;

	mutex_lock(&memory_lowpower_mutex);

	if (cma_pages) {
		ret = cma_release(cma, cma_pages, count);
		if (!ret) {
			pr_err("%s incorrect pages: %p(%lx)\n",
					__func__,
					cma_pages, page_to_pfn(cma_pages));
			return -EINVAL;
		}
		cma_pages = 0;
	}

	mutex_unlock(&memory_lowpower_mutex);

	return 0;
}

static int memory_lowpower_init(struct reserved_mem *rmem)
{
	int ret;

	pr_debug("%s, name: %s, base: 0x%pa, size: 0x%pa\n",
		 __func__, rmem->name,
		 &rmem->base, &rmem->size);

	/* init cma area */
	ret = cma_init_reserved_mem(rmem->base, rmem->size , 0, &cma);

	if (ret) {
		pr_err("%s cma failed, ret: %d\n", __func__, ret);
		return 1;
	}

	return 0;
}

RESERVEDMEM_OF_DECLARE(memory_lowpower, "mediatek,memory-lowpower",
			memory_lowpower_init);

#ifdef CONFIG_MTK_MEMORY_LOWPOWER_DEBUG
static int memory_lowpower_show(struct seq_file *m, void *v)
{
	phys_addr_t cma_base = cma_get_base(cma);
	phys_addr_t cma_end = cma_base + cma_get_size(cma);

	mutex_lock(&memory_lowpower_mutex);

	if (cma_pages)
		seq_printf(m, "cma collected cma_pages: %p\n", cma_pages);
	else
		seq_puts(m, "cma freed NULL\n");

	mutex_unlock(&memory_lowpower_mutex);

	seq_printf(m, "cma info: [%pa-%pa] (0x%lx)\n",
			&cma_base, &cma_end,
			cma_get_size(cma));

	return 0;
}

static int memory_lowpower_open(struct inode *inode, struct file *file)
{
	return single_open(file, &memory_lowpower_show, NULL);
}

static ssize_t memory_lowpower_write(struct file *file, const char __user *buffer,
					size_t count, loff_t *ppos)
{
	static char state;

	if (count > 0) {
		if (get_user(state, buffer))
			return -EFAULT;
		state -= '0';
		pr_alert("%s state = %d\n", __func__, state);
		if (state) {
			/* collect cma */
			get_memory_lowpower_cma();
		} else {
			/* undo collection */
			put_memory_lowpower_cma();
		}
	}

	return count;
}

static const struct file_operations memory_lowpower_fops = {
	.open		= memory_lowpower_open,
	.write		= memory_lowpower_write,
	.read		= seq_read,
	.release	= single_release,
};

static int __init memory_lowpower_debug_init(void)
{
	struct dentry *dentry;

	if (!cma) {
		pr_err("memory-lowpower cma is not inited\n");
		return 1;
	}

	dentry = debugfs_create_file("memory-lowpower", S_IRUGO, NULL, NULL,
					&memory_lowpower_fops);
	if (!dentry)
		pr_warn("Failed to create debugfs memory_lowpower_debug_init file\n");

	return 0;
}

late_initcall(memory_lowpower_debug_init);
#endif /* CONFIG_MTK_MEMORY_LOWPOWER_DEBUG */

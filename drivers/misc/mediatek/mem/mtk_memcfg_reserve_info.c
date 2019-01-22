#include <linux/kernel.h>
#include <linux/seq_file.h>
#include <linux/memblock.h>
#include "mtk_memcfg_reserve_info.h"

#define DRAM_ALIGN_SIZE 0x20000000

static int mtk_memcfg_total_raserve_show(struct seq_file *m, void *v)
{
	struct sysinfo i;
	phys_addr_t dram_start, dram_end, dram_size;
	phys_addr_t memtotal;

#define K(x) ((x) << (PAGE_SHIFT - 10))
	si_meminfo(&i);
	memtotal = (phys_addr_t)K(i.totalram);
	dram_start = __ALIGN_MASK(memblock_start_of_DRAM(), DRAM_ALIGN_SIZE);
	dram_end = ALIGN(memblock_end_of_DRAM(), DRAM_ALIGN_SIZE);
	dram_size = dram_end - dram_start;

	seq_printf(m, "%llu kB\n", (unsigned long long)(K(dram_size >> PAGE_SHIFT)) - (unsigned long long)memtotal);

	return 0;
}

static int mtk_memcfg_raserve_over_1M_show(struct seq_file *m, void *v)
{
	return 0;
}

static int mtk_memcfg_total_reserve_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtk_memcfg_total_raserve_show, NULL);
}

static int mtk_memcfg_reserve_over_1M_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtk_memcfg_raserve_over_1M_show, NULL);
}

static const struct file_operations mtk_memcfg_total_raserve_operations = {
	.open = mtk_memcfg_total_reserve_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations mtk_memcfg_reserve_over_1M_operations = {
	.open = mtk_memcfg_reserve_over_1M_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

int __init mtk_memcfg_reserve_info_init(struct proc_dir_entry *mtk_memcfg_dir)
{
	struct proc_dir_entry *entry = NULL;

	if (!mtk_memcfg_dir) {
		pr_err("/proc/mtk_memcfg not exist");
		return 0;
	}

	entry = proc_create("total_reserve",
			    S_IRUGO | S_IWUSR, mtk_memcfg_dir,
			    &mtk_memcfg_total_raserve_operations);
	if (!entry)
		pr_warn("create total_reserve_memory proc entry failed\n");

	entry = proc_create("reserve_over_1M",
			    S_IRUGO | S_IWUSR, mtk_memcfg_dir,
			    &mtk_memcfg_reserve_over_1M_operations);
	if (!entry)
		pr_warn("create reserve_over_1M proc entry failed\n");

	return 0;
}


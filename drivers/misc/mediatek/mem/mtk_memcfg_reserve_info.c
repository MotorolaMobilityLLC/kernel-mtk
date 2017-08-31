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

#include <linux/kernel.h>
#include <linux/seq_file.h>
#include <linux/memblock.h>
#include <linux/of_fdt.h>
#include <linux/of.h>
#include <linux/sort.h>
#include <linux/slab.h>
#include <asm/setup.h>
#include <linux/mm.h>
#include "mtk_memcfg_reserve_info.h"

#define DRAM_ALIGN_SIZE 0x20000000

static void merge_same_reserved_memory(struct reserved_mem_ext *reserved_mem, int reserved_mem_count)
{
	int cur, tmp;
	struct reserved_mem_ext *cur_mem, *tmp_mem;

	for (cur = 0; cur < reserved_mem_count; cur++) {
		cur_mem = &reserved_mem[cur];
		for (tmp = cur + 1; tmp < reserved_mem_count; tmp++) {
			tmp_mem = &reserved_mem[tmp];
			if (!strcmp(cur_mem->name, tmp_mem->name)) {
				cur_mem->size += tmp_mem->size;
				tmp_mem->size = 0;
			}
		}
	}
}

int mtk_memcfg_pares_reserved_memory(struct reserved_mem_ext *reserved_mem, int reserved_mem_count)
{
	int page_num, reserved_count = reserved_mem_count;
	int index;
	unsigned long start_pfn, end_pfn, start, end, page_count;
	struct page *page;
	struct reserved_mem_ext *rmem;

	for (index = 0; index < reserved_count; index++) {
		rmem = &reserved_mem[index];
		start_pfn = __phys_to_pfn(rmem->base);
		end_pfn = __phys_to_pfn(rmem->base + rmem->size);

		if (start_pfn > end_pfn) {
			pr_info("start_pfn %lu > end_pfn %lu\n", start_pfn, end_pfn);
			pr_info("reserved_mem: %s, base: %llu, size: %llu, nomap: %d\n",
					rmem->name,
					rmem->base,
					rmem->size,
					rmem->nomap);
			WARN_ON(start_pfn > end_pfn);
		}

		page_count = end_pfn - start_pfn;
		start = 0;
		end = 0;

		if (!pfn_valid(start_pfn)) {
			reserved_mem[index].nomap =
				RESERVED_NO_MAP;
			continue;
		}

		for (page_num = 0; page_num < page_count; page_num++) {
			page = pfn_to_page(start_pfn + page_num);
			if (PageReserved(page)) {
				if (start == 0)
					start = start_pfn + page_num;
			} else {
				if (start != 0) {
					struct reserved_mem_ext *tmp =
						&reserved_mem[reserved_count];

					reserved_count += 1;

					if (reserved_count > MAX_RESERVED_REGIONS)
						return -1;

					end = start_pfn + page_num;
					tmp->base =
						page_to_phys(pfn_to_page(start));
					tmp->size = page_to_phys(pfn_to_page(end)) -
						page_to_phys(pfn_to_page(start));
					tmp->name = rmem->name;
					tmp->nomap = RESERVED_MAP;
					end = 0;
					start = 0;

					tmp = &reserved_mem[index];
					tmp->base = 0;
					tmp->size = 0;
				}
			}
		}
		if (start != 0)
			end = start_pfn + page_count;
	}
	return reserved_count;
}

int mtk_memcfg_get_reserved_memory(struct reserved_mem_ext *reserved_mem)
{
	int i = 0, reserved_count = 0;
	struct reserved_mem tmp;

	reserved_count = get_reserved_mem_count();
	pr_info("reserved_mem_count: %d\n", reserved_count);

	if (reserved_count > MAX_RESERVED_REGIONS) {
		pr_info("reserved_count: %d, over limit MAX_RESERVED_REGIONS : %d\n",
				reserved_count, MAX_RESERVED_REGIONS);
		return 0;
	}

	for (i = 0; i < reserved_count; i++) {
		tmp = get_reserved_mem(i);
		reserved_mem[i].name = tmp.name;
		reserved_mem[i].base = tmp.base;
		reserved_mem[i].size = tmp.size;
		reserved_mem[i].nomap = RESERVED_MAP;
	}
	return reserved_count;
}

int reserved_mem_ext_compare(const void *p1, const void *p2)
{
	if (((struct reserved_mem_ext *)p1)->base > ((struct reserved_mem_ext *)p2)->base)
		return 1;
	return -1;
}

void clean_reserved_mem_by_name(struct reserved_mem_ext *reserved_mem, int reserved_count, const char *name)
{
	int i = 0;

	for (i = 0; i < reserved_count; i++) {
		if (strcmp(name, (const char *)reserved_mem[i].name) == 0) {
			reserved_mem[i].size = 0;
			reserved_mem[i].base = 0;
		}
	}
}

static unsigned long long get_dram_size(void)
{
	phys_addr_t dram_start, dram_end, dram_size;

	dram_start = __ALIGN_MASK(memblock_start_of_DRAM(), DRAM_ALIGN_SIZE);
	dram_end = ALIGN(memblock_end_of_DRAM(), DRAM_ALIGN_SIZE);
	dram_size = dram_end - dram_start;

	return (unsigned long long)dram_size;
}

static unsigned long long get_memtotal(void)
{
	struct sysinfo i;
	unsigned long long memtotal;

	si_meminfo(&i);
	memtotal = i.totalram << PAGE_SHIFT;

	return memtotal;
}

static int mtk_memcfg_total_reserve_show(struct seq_file *m, void *v)
{
	unsigned long long dram_size, memtotal;

#define K(x) ((x) >> (10))
	memtotal = get_memtotal();
	dram_size = get_dram_size();

	seq_printf(m, "%llu kB\n", (unsigned long long)(K(dram_size)) - (unsigned long long)K(memtotal));

	return 0;
}

static int mtk_memcfg_reserve_memory_show(struct seq_file *m, void *v)
{
	int i = 0;
	int reserved_count;
	unsigned long long dram_size, memtotal, kernel_other, vmemmap_actual;
	unsigned long long non_kernel_reserve = 0;
	struct reserved_mem_ext *reserved_mem = NULL;
	struct reserved_mem_ext *tmp = NULL;
	char *path = "/memory";
	struct device_node *dt_node;
	const struct mblock_info *mb_info = NULL;

	dt_node = of_find_node_by_path(path);
	if (!dt_node) {
		seq_puts(m, "Failed to get dts not \"memory\"\n");
		return 0;
	}

	mb_info = of_get_property(dt_node, "mblock_info", NULL);
	if (!mb_info || !(mb_info->mblock_magic == 0x99999999 && mb_info->mblock_version >= 2)) {
		seq_puts(m, "Memory layout does not support mblock version < 2\n");
		return 0;
	}

	reserved_mem = kcalloc(MAX_RESERVED_REGIONS,
				sizeof(struct reserved_mem_ext),
				GFP_KERNEL);
	if (!reserved_mem) {
		seq_puts(m, "Can't get memory to parse reserve memory.(Is it OOM?)\n");
		return 0;
	}

	reserved_count = mtk_memcfg_get_reserved_memory(reserved_mem);
	if (reserved_count == 0) {
		pr_info("Can't get reserve memory.\n");
		kfree(reserved_mem);
		return 0;
	}

	reserved_count = mtk_memcfg_pares_reserved_memory(reserved_mem, reserved_count);
	if (reserved_count <= 0 || reserved_count > MAX_RESERVED_REGIONS) {
		seq_printf(m, "reserved_count(%d) over limit after parsing!\n",
				reserved_count);
		kfree(reserved_mem);
		return 0;
	}

	clean_reserved_mem_by_name(reserved_mem, reserved_count, "zone-movable-cma-memory");
	merge_same_reserved_memory(reserved_mem, reserved_count);

	sort(reserved_mem, reserved_count,
			sizeof(struct reserved_mem_ext),
			reserved_mem_ext_compare, NULL);

	for (i = 0; i < reserved_count; i++) {
		tmp = &reserved_mem[i];
		if (tmp->size != 0) {
			non_kernel_reserve += tmp->size;
			if (tmp->nomap || strstr(tmp->name, "ccci"))
				seq_puts(m, "*");
			seq_printf(m, "%s: %llu\n", tmp->name, tmp->size);
		}
	}

	memtotal = get_memtotal();
	dram_size = get_dram_size();
	vmemmap_actual = (unsigned long)virt_to_page(high_memory) -
			 (unsigned long)phys_to_page(memblock_start_of_DRAM());
	kernel_other = dram_size - memtotal - non_kernel_reserve -
		       kernel_reserve_meminfo.kernel_code -
		       kernel_reserve_meminfo.rwdata -
		       kernel_reserve_meminfo.rodata -
		       kernel_reserve_meminfo.bss -
		       vmemmap_actual;

	seq_printf(m, "kernel(text): %llu\n", kernel_reserve_meminfo.kernel_code);
	seq_printf(m, "kernel(data): %llu\n", kernel_reserve_meminfo.rwdata +
					      kernel_reserve_meminfo.rodata +
					      kernel_reserve_meminfo.bss);
	seq_printf(m, "kernel(page): %llu\n", vmemmap_actual);
	seq_printf(m, "kernel(other): %llu\n", kernel_other);

	return 0;
}

static int mtk_memcfg_total_reserve_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtk_memcfg_total_reserve_show, NULL);
}

static int mtk_memcfg_reserve_memory_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtk_memcfg_reserve_memory_show, NULL);
}

static const struct file_operations mtk_memcfg_total_reserve_operations = {
	.open = mtk_memcfg_total_reserve_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations mtk_memcfg_reserve_memory_operations = {
	.open = mtk_memcfg_reserve_memory_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

int __init mtk_memcfg_reserve_info_init(struct proc_dir_entry *mtk_memcfg_dir)
{
	struct proc_dir_entry *entry = NULL;

	if (!mtk_memcfg_dir) {
		pr_info("/proc/mtk_memcfg not exist");
		return 0;
	}

	entry = proc_create("total_reserve",
			    S_IRUGO | S_IWUSR, mtk_memcfg_dir,
			    &mtk_memcfg_total_reserve_operations);
	if (!entry)
		pr_info("create total_reserve_memory proc entry failed\n");

	entry = proc_create("reserve_memory",
			    S_IRUGO | S_IWUSR, mtk_memcfg_dir,
			    &mtk_memcfg_reserve_memory_operations);
	if (!entry)
		pr_info("create reserve_memory proc entry failed\n");

	return 0;
}


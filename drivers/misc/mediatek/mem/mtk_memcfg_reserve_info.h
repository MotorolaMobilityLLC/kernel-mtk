#ifndef __MTK_MEMCFG_RESERVE_INFO__
#define __MTK_MEMCFG_RESERVE_INFO__
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/of_reserved_mem.h>

#define MAX_RESERVED_REGIONS	40
#define RESERVED_NO_MAP 1
#define RESERVED_MAP 0

extern struct kernel_reserve_meminfo kernel_reserve_meminfo;

struct reserved_mem_ext {
	const char	*name;
	unsigned long long base;
	unsigned long long size;
	int		nomap;
};

int __init mtk_memcfg_reserve_info_init(struct proc_dir_entry *mtk_memcfg_dir);
int mtk_memcfg_pares_reserved_memory(struct reserved_mem_ext *reserved_mem, int reserved_mem_count);
int mtk_memcfg_get_reserved_memory(struct reserved_mem_ext *reserved_mem);
int reserved_mem_ext_compare(const void *p1, const void *p2);
void clean_reserved_mem_by_name(struct reserved_mem_ext *reserved_mem, int reserved_count, const char *name);

#endif /* end of __MTK_MEMCFG_RESERVE_INFO__ */
